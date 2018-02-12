TARGET = matron
include ../version.mk

.PHONY = clean

CC = gcc
LD = gcc

CFLAGS += $(VERSION)
CFLAGS += -std=gnu11
CFLAGS += -Wall -Wextra -Werror
CFLAGS += -g

INC =  -Isrc -Isrc/device -Ilua -Isrc/hardware
INC += -I/usr/include/libevdev-1.0/libevdev
INC += -I/usr/include/cairo -I/usr/include/freetype2

LDFLAGS =

LIB = -ldl -llo -lpthread -lmonome -ludev -levdev -lm -lcairo -lfreetype

STATIC = lua/liblua.a

OBJ_DIR = obj
SRC_DIR = src

_OBJ = main.o \
	args.o \
	device/device.o \
	device/device_hid.o \
	device/device_list.o \
	device/device_monitor.o \
	device/device_monome.o \
    device/osc.o \
	events.o \
	hardware/battery.o \
	hardware/gpio.o \
	hardware/i2c.o \
	hardware/screen.o \
	input.o \
	lua_eval.o \
	metro.o \
	oracle.o \
	weaver.o \

OBJ = $(patsubst %,$(OBJ_DIR)/%,$(_OBJ))

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	@ mkdir -p $(OBJ_DIR)/device
	@ mkdir -p $(OBJ_DIR)/hardware
	$(CC) -c $(CFLAGS) $(INC) $< -o $@  

$(TARGET): $(OBJ) $(STATIC)
	$(LD) $(LDFLAGS) $(OBJ) -o $@ $(STATIC) $(LIB)

lua/liblua.a:
	cd lua && make 

clean:
	rm $(TARGET)
	rm -rf $(OBJ_DIR)
	cd lua && make clean
