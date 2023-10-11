# dwmblocks2
Fork of dwmblocks. It is **not** a drop-in replacement.

## Usage
```
dwmblocks2 & # put this line in your xinitrc
```

## Configuration
Edit `blocks.h`.
By default it uses all blocks in `blocks/` directory,
plus the clock block.
You can use external programs or C functions of
prototype `void function_name(int, Output *)`,
defined in `block_functions.c` and declared in `block_functions.h`.
Checkout the clock block for an example.

## Notes
- Each command specificed on `blocks.h` is not interpreted by the shell,
  since it is probably itself a script, so the overhead and the loss
  of readability is not worth the "flexibility"
- A signal number, specified through an environmental variable,
  is mandatory for each block.
- Blocks whose interval is set to 0 will only be updated through signals,
  (including those send by dwm through clicks).

## Clicks
You need to apply the patch `dwm-statuscmd.diff` on dwm
for mouse clicks to work.
Then dwmblocks2 will pass the button as the first argument.
```
case $1 in
    1) echo "clicked left button";;
    2) echo "clicked middle button";;
    3) echo "clicked right button";;
    4) echo "scroll up";;
    5) echo "scroll down";;
    6) echo "shift + left button";;
    7) echo "control + left button";;
    "") echo "no click";;
esac
```

## Instalation
```
$ git clone https://github.com/lucas-mior/dwmblocks2
$ cd dwmblocks2
$ make
$ sudo make install
```
In order to use the default blocks, you have to put them in your
`PATH` and also set the environmental variables (see `blocks.h`).

## Differences from original dwmblocks
- It's much lighter, since the shell is avoided on every command spawned.
- A hard to reproduce bug in which `dwmblocks` would freeze has been fixed.
- Clicks are passed through the first argument, not the `BLOCK_BUTTON`
  environment variable.
- Signals are mandatory and set through environment variables, so one
  can more easily keep dwmblocks signals in sync with scripts which update
  the contents of the bar.
- There is the possibility of using C functions as blocks, which is
  useful for the included clock block, which runs every second, avoiding the
  overhead of forking and executing a new process.

## License
dwmblocks2 is licensed under GPLv2,
block scripts in `blocks/` are licensed under AGPL.
