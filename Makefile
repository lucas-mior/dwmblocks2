output: dwmblocks.o
	gcc dwmblocks.o -lX11 -o dwmblocks
dwmblocks.o: dwmblocks.c config.h
	gcc -c -lX11 dwmblocks.c
clean:
	rm *.o *.gch dwmblocks
install: output
	mkdir -p /usr/local/bin
	cp -f dwmblocks /usr/local/bin
	chmod 755 /usr/local/bin/dwmblocks
