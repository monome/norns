all: matron ws-wrapper

.PHONY: matron ws-wrapper clean

matron: 
	cd matron && make

ws-wrapper:
	cd ws-wrapper && make

docs:
	ldoc .

clean:
	cd matron && make clean
	cd ws-wrapper && make clean
