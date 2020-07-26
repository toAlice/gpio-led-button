CC=cc
LIBS=-lgpiod -lpthread
CFLAGS=-Wall -DPRINT_LINES

all: gpio-led-button.c
	$(CC) gpio-led-button.c $(LIBS) $(CFLAGS) -o gpio-led-button


clean: 
	-rm -f gpio-led-button


