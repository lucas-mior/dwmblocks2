#!/usr/bin/env python3
"""Integration checks for dwmblocks2's scheduling and status publication.

Build the program first, then run this test on Linux with Xvfb installed.
"""

import ctypes
import os
from pathlib import Path
import re
import signal
import subprocess
import tempfile
import time


SOURCE_ROOT = Path(__file__).resolve().parent.parent
BINARY = SOURCE_ROOT / "bin" / SOURCE_ROOT.name
PROPERTY_NOTIFY = 28
PROPERTY_CHANGE_MASK = 1 << 22


class XEvent(ctypes.Structure):
    _fields_ = [("type", ctypes.c_int), ("rest", ctypes.c_byte * 188)]


def wait_for(check, timeout=2.0):
    start = time.monotonic()
    while time.monotonic() - start < timeout:
        if check():
            return time.monotonic() - start
        time.sleep(0.005)
    raise AssertionError(f"Update not observed after {timeout} seconds")


def main():
    if not BINARY.exists():
        raise SystemExit(f"Build the executable first: {BINARY}")

    xlib = ctypes.CDLL("libX11.so.6")
    xlib.XOpenDisplay.argtypes = [ctypes.c_char_p]
    xlib.XOpenDisplay.restype = ctypes.c_void_p
    xlib.XDefaultRootWindow.argtypes = [ctypes.c_void_p]
    xlib.XDefaultRootWindow.restype = ctypes.c_ulong
    xlib.XFetchName.argtypes = [ctypes.c_void_p, ctypes.c_ulong,
                               ctypes.POINTER(ctypes.c_char_p)]
    xlib.XFetchName.restype = ctypes.c_int
    xlib.XFree.argtypes = [ctypes.c_void_p]
    xlib.XInternAtom.argtypes = [ctypes.c_void_p, ctypes.c_char_p,
                                ctypes.c_int]
    xlib.XInternAtom.restype = ctypes.c_ulong
    xlib.XGetSelectionOwner.argtypes = [ctypes.c_void_p, ctypes.c_ulong]
    xlib.XGetSelectionOwner.restype = ctypes.c_ulong
    xlib.XGetWindowProperty.argtypes = [
        ctypes.c_void_p, ctypes.c_ulong, ctypes.c_ulong, ctypes.c_long,
        ctypes.c_long, ctypes.c_int, ctypes.c_ulong,
        ctypes.POINTER(ctypes.c_ulong), ctypes.POINTER(ctypes.c_int),
        ctypes.POINTER(ctypes.c_ulong), ctypes.POINTER(ctypes.c_ulong),
        ctypes.POINTER(ctypes.POINTER(ctypes.c_ulong)),
    ]
    xlib.XGetWindowProperty.restype = ctypes.c_int
    xlib.XSelectInput.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.c_long]
    xlib.XPending.argtypes = [ctypes.c_void_p]
    xlib.XPending.restype = ctypes.c_int
    xlib.XNextEvent.argtypes = [ctypes.c_void_p, ctypes.POINTER(XEvent)]
    xlib.XCloseDisplay.argtypes = [ctypes.c_void_p]
    libc = ctypes.CDLL(None)
    libc.sigqueue.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int]

    contents = (SOURCE_ROOT / "blocks.h").read_text()
    blocks = re.findall(
        r'\{(?:NULL|block_clock),\s*(NULL|"[^"]+")\s*,\s*'
        r'"(DWMBLOCKS2_[A-Z0-9]+)"\s*,\s*(\d+)\}', contents
    )
    if not blocks:
        raise RuntimeError("Could not read block configuration")

    with tempfile.TemporaryDirectory(prefix="dwmblocks2-test-") as tmp:
        temp = Path(tmp)
        script = temp / "fakeblock.sh"
        script.write_text(
            '#!/bin/sh\n'
            'case "$0" in\n'
            '    *volume*) printf "VOLUME-%s-%s\\n" "$1" '
            '"$(cat "$TEST_VOLUME")" ;;\n'
            '    *network*) printf "NETWORK-%s\\n" "$1" ;;\n'
            '    *ip.sh*) printf "IP-%s\\n" "$1" ;;\n'
            '    *joystick*) sleep 0.35; echo JOYSTICK ;;\n'
            '    *phone*) sleep 0.2; echo PHONE ;;\n'
            '    *) printf "%s-%s\\n" "${0##*/}" "$1" ;;\n'
            'esac\n'
        )
        script.chmod(0o755)
        (temp / "volume").write_text("zero")

        env = dict(os.environ)
        env["PATH"] = f'{tmp}:{env.get("PATH", "")}'
        env["TEST_VOLUME"] = str(temp / "volume")
        for i, (command, variable, _interval) in enumerate(blocks):
            if command != "NULL":
                (temp / command.strip('"')).symlink_to(script)
            env[variable] = str(i + 1)

        xvfb = subprocess.Popen(
            ["Xvfb", "-displayfd", "1", "-nolisten", "tcp",
             "-screen", "0", "800x600x24"],
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
        )
        daemon = None
        display = None
        try:
            display_number = xvfb.stdout.readline().decode().strip()
            if not display_number:
                raise RuntimeError("Xvfb failed to start")
            env["DISPLAY"] = f":{display_number}"
            display = xlib.XOpenDisplay(env["DISPLAY"].encode())
            if not display:
                raise RuntimeError("Cannot open virtual display")
            root = xlib.XDefaultRootWindow(display)
            xlib.XSelectInput(display, root, PROPERTY_CHANGE_MASK)
            pid_atom = xlib.XInternAtom(display, b"_DWMBLOCKS2_PID", 0)
            owner_atom = xlib.XInternAtom(display, b"_DWMBLOCKS2_OWNER", 0)
            cardinal = xlib.XInternAtom(display, b"CARDINAL", 0)

            def status_owner():
                return xlib.XGetSelectionOwner(display, owner_atom)

            def pid_record():
                actual_type = ctypes.c_ulong()
                actual_format = ctypes.c_int()
                count = ctypes.c_ulong()
                remaining = ctypes.c_ulong()
                values = ctypes.POINTER(ctypes.c_ulong)()
                result = xlib.XGetWindowProperty(
                    display, root, pid_atom, 0, 2, 0, cardinal,
                    ctypes.byref(actual_type), ctypes.byref(actual_format),
                    ctypes.byref(count), ctypes.byref(remaining),
                    ctypes.byref(values),
                )
                if result != 0 or not values:
                    return None
                try:
                    if (actual_type.value != cardinal
                            or actual_format.value != 32
                            or count.value != 2 or remaining.value != 0):
                        return None
                    return (values[0], values[1])
                finally:
                    xlib.XFree(ctypes.cast(values, ctypes.c_void_p))

            def verified_pid():
                record = pid_record()
                if record and record[1] and record[1] == status_owner():
                    return record[0]
                return None

            def status():
                name = ctypes.c_char_p()
                if xlib.XFetchName(display, root, ctypes.byref(name)) == 0:
                    return ""
                result = ctypes.string_at(name).decode("utf-8", "replace")
                xlib.XFree(ctypes.cast(name, ctypes.c_void_p))
                return result

            def changed_properties():
                count = 0
                while xlib.XPending(display):
                    event = XEvent()
                    xlib.XNextEvent(display, ctypes.byref(event))
                    if event.type == PROPERTY_NOTIFY:
                        count += 1
                return count

            def click(variable, button):
                rt_signal = int(signal.SIGRTMIN) + int(env[variable])
                value = ((rt_signal + 1) << 3) | button
                if libc.sigqueue(daemon.pid, int(signal.SIGUSR1), value):
                    raise OSError(ctypes.get_errno(), "sigqueue failed")

            def clock_seconds():
                match = re.search(r"📅 [^\x01]*?\d\d:\d\d:(\d\d)",
                                  status())
                if match:
                    return match.group(1)
                return None

            daemon = subprocess.Popen(
                [str(BINARY)], env=env, stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )
            wait_for(lambda: "VOLUME-0-zero" in status())
            wait_for(lambda: verified_pid() == daemon.pid)

            # The slow initial scripts must not hold up a volume update.
            (temp / "volume").write_text("one")
            os.kill(daemon.pid,
                    int(signal.SIGRTMIN) + int(env["DWMBLOCKS2_VOLUME"]))
            latency = wait_for(lambda: "VOLUME-0-one" in status())
            assert latency < 0.4, f"Volume signal took {latency:.3f}s"

            click("DWMBLOCKS2_VOLUME", 4)
            latency = wait_for(lambda: "VOLUME-4-one" in status())
            assert latency < 0.4, f"Mouse signal took {latency:.3f}s"

            # Several blocks may share one real-time signal.
            click("DWMBLOCKS2_NETWORK", 3)
            wait_for(lambda: "NETWORK-3" in status() and "IP-3" in status())

            # Periodic clock updates must continue while signals keep arriving.
            first = clock_seconds()
            seen = {first}
            start = time.monotonic()
            while time.monotonic() - start < 2.3:
                os.kill(daemon.pid, int(signal.SIGRTMIN)
                        + int(env["DWMBLOCKS2_VOLUME"]))
                time.sleep(0.025)
                seen.add(clock_seconds())
            assert len(seen - {None}) >= 2, "Periodic clock was starved"

            # Re-running an unchanged block must not republish the X property.
            wait_for(lambda: "JOYSTICK" in status())
            for _ in range(3):
                previous_clock = clock_seconds()
                changed_properties()
                os.kill(daemon.pid, int(signal.SIGRTMIN)
                        + int(env["DWMBLOCKS2_VOLUME"]))
                time.sleep(0.12)
                changed = changed_properties()
                if clock_seconds() == previous_clock:
                    assert changed == 0, "Unchanged status caused a redraw"
                    break
            else:
                raise AssertionError("Could not test unchanged publication")

            assert daemon.poll() is None, "dwmblocks2 exited unexpectedly"

            # A crashed process leaves a root property, but no live owner.
            original = pid_record()
            daemon.kill()
            daemon.wait(timeout=2)
            wait_for(lambda: status_owner() == 0)
            assert pid_record() == original, "Expected stale root PID property"
            assert verified_pid() is None, "Stale PID was considered live"

            # Starting a new instance replaces the identity automatically.
            daemon = subprocess.Popen(
                [str(BINARY)], env=env, stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )
            wait_for(lambda: verified_pid() == daemon.pid)
            assert pid_record()[0] != original[0], "PID did not change"
            (temp / "volume").write_text("restarted")
            click("DWMBLOCKS2_VOLUME", 5)
            wait_for(lambda: "VOLUME-5-restarted" in status())

            print(f"PASS: event loop (volume latency {latency*1000:.0f}ms; "
                  "clicks, shared signals, periodic clock, unchanged status; "
                  "PID lifetime and restart)")
        finally:
            if daemon is not None:
                daemon.terminate()
                try:
                    daemon.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    daemon.kill()
                    daemon.wait()
            if display:
                xlib.XCloseDisplay(display)
            xvfb.terminate()
            try:
                xvfb.wait(timeout=2)
            except subprocess.TimeoutExpired:
                xvfb.kill()
                xvfb.wait()


if __name__ == "__main__":
    main()
