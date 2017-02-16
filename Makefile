all: norns matron
.PHONY: matron clean

CFLAGS = -g -std=gnu11

SRC = norns.c

norns: $(SRC)
	gcc $(CFLAGS) $(SRC) -o norns -lpthread

matron: 
	cd matron && make

clean:
	rm norns
