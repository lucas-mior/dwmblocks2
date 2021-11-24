//Modify this file to change what commands output to your statusbar, and recompile using the make command.
static const Block blocks[] = {
	/*Icon*/  /*Command*/    /*Update Interval*/    /*Update Signal*/
	{"", "cat /tmp/ric 2>/dev/null",   0,    10},
	{"", "0notmounted.sh",             0,    11},
	{"", "0mounted.sh",                0,    12},
	{"", "0phone.sh",                  0,    13},
	{"", "musica.sh",                  0,    14},
	{"", "vol.sh",                     0,    15},
	{"", "mic.sh",                     0,    16},
	{"", "bril.sh",                    0,    17},
	{"", "news.sh",                    0,    18},
	{"", "horario.sh",                 1,    19},
	{"", "1memory.sh",                60,    20},
	{"", "1cpu.sh",                   60,    21},
	{"", "bateria.sh",                60,    22},
};

//Sets delimiter between status commands. NULL character ('\0') means no delimiter.
static char *delim = " ";

// Have dwmblocks automatically recompile and run when you edit this file in
// vim with the following line in your vimrc/init.vim:

// autocmd BufWritePost ~/.local/src/dwmblocks/config.h !cd ~/.local/src/dwmblocks/; sudo make install && { killall -q dwmblocks;setsid dwmblocks & }
