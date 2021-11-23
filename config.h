//Modify this file to change what commands output to your statusbar, and recompile using the make command.
static const Block blocks[] = {
	/*Icon*/  /*Command*/    /*Update Interval*/    /*Update Signal*/
	{"", "cat /tmp/ric 2>/dev/null",   0,    16},
	{"", "0notmounted.sh",             0,    11},
	{"", "0mounted.sh",                0,    12},
	{"", "0phone.sh",                  0,    13},
	{"", "musica.sh",                  0,     1},
	{"", "vol.sh",                     0,     2},
	{"", "mic.sh",                     0,     3},
	{"", "bril.sh",                    0,     4},
	{"", "news.sh",                    0,     8},
	{"", "horario.sh",                 1,     0},
	{"", "1memory.sh",                60,     0},
	{"", "1cpu.sh",                   60,     0},
	{"", "bateria.sh",                60,     5},
};

//Sets delimiter between status commands. NULL character ('\0') means no delimiter.
static char *delim = " ";

// Have dwmblocks automatically recompile and run when you edit this file in
// vim with the following line in your vimrc/init.vim:

// autocmd BufWritePost ~/.local/src/dwmblocks/config.h !cd ~/.local/src/dwmblocks/; sudo make install && { killall -q dwmblocks;setsid dwmblocks & }
