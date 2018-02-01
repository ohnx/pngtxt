CFLAGS+=-Wall -Werror -pedantic -ansi

pngtxt: pngtxt.c crc32.c
	$(CC) $(CFLAGS) $^ -o pngtxt

.PHONY: reset
reset:
	cp pngs/png.bak.png pngs/png.png

.PHONY: w1k w10k w1m
w1k: reset
	./pngtxt write pngs/png.png texts/loremipsum.txt
	pngcheck pngs/png.png

w10k: reset
	./pngtxt write pngs/png.png texts/mansfieldpark-ch1.txt
	pngcheck pngs/png.png

w1m: reset
	./pngtxt write pngs/png.png texts/mansfieldpark-full.txt
	pngcheck pngs/png.png
