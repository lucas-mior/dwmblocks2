# dwmblocks2
Fork of dwmblocks. It is **not** a drop-in replacement.

## Usage
```
dwmblocks2 & # put this line in your xinitrc
```

## Configuration
Edit `blocks.h`.
By default it uses all blocks in `blocks/` directory,
plus the clock block, which is part of dwmblocks2 itself.
You are encouraged to edit `main.c` to suit your needs.

## Clicks
You need to apply the patch `dwm-statuscmd.diff` on dwm
for mouse clicks to work.
Then dwmblocks2 will export `$DWMBLOCKS2_BUTTON` to your block program.
```
case $DWMBLOCKS2_BUTTON in
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
You also need to set the environment variable `DWMBLOCKS2_CLOCK`
```
export DWMBLOCKS2_CLOCK=16 # add this line to your .bash_profile or equivalent
```
In order to use the blocks in `blocks/`, you have to put them in your
`PATH` and also set the environmental variables (see `blocks.h`).

## License
dwmblocks2 is licensed under GPLv2,
block scripts in `blocks/` are licensed under AGPL.
