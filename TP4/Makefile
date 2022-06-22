CC=gcc
CFLAGS=-Wall -Werror -g -pthread -lrt
PROGS=servertp4 clientetp4


all:$(PROGS)
%: %..c
	$(CC) -o $@ $< %(CFLAGS)

clean:
	rm -rf $(PROGS)
