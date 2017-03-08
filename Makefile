all: maiden matron

.PHONY: maiden matron clean

maiden:
	cd maiden && make

matron: 
	cd matron && make

clean:
	cd maiden && make clean
	cd matron && make clean
