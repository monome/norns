all: norns matron
.PHONY: matron clean

CC = gcc
LD = gcc

CFLAGS = -g -std=gnu11

SRC = norns.c

LIB = -lpthread -lreadline

norns: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o norns $(LIB)

matron: 
	cd matron && make

clean:
	rm norns
	cd matron && make clean
