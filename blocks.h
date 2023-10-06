static Block blocks[] = {
	/*Command*/    /*Update Interval*/    /*Update Signal*/
	{"cat /tmp/ric 2>/dev/null",        0,    10},
	{"clipout.sh",                      0,    11},
	{"0ydl.sh",                         0,    12},
	{"0notmounted.sh",                  0,    13},
	{"0mounted.sh",                     0,    14},
	{"0phone.sh",                       0,    15},
	{"musica.sh",                       0,    16},
	{"vol.sh",                          0,    17},
	{"mic.sh",                          0,    18},
	{"bril.sh",                         0,    19},
	{"rorario",                         1,    21},
	{"1memory.sh",                     60,    22},
	{"1temp.sh",                       60,    23},
	{"1internet.awk"
	 " /sys/class/net/w*/flags"
	 " /sys/class/net/*/operstate",     0,    27},
	{"1bluetooth.sh",                  80,    25},
	{"bateria.awk "
		 "/sys/class/power_*/BAT0/uevent", 60,    26},
};
