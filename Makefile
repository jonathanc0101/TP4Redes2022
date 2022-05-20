CC=gcc
CFLAGS=-Wall -Werror -g -lpthread -lrt
PROGS=holamundo cliente


all:$(PROGS)
%: %..c
	$(CC) -o $@ $< %(CFLAGS)

clean:
	rm -rf $(PROGS)
