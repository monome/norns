all: maiden matron ipc-wrapper

.PHONY: maiden matron ipc-wrapper clean

maiden:
	cd maiden && make

matron: 
	cd matron && make

ipc-wrapper:
	cd ipc-wrapper && make

clean:
	cd maiden && make clean
	cd matron && make clean
	cd ipc-wrapper && make clean
