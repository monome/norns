TARGET = matron
include ../version.mk

.PHONY = clean

CC = gcc
LD = gcc

CFLAGS += $(VERSION)
CFLAGS += -std=gnu11
CFLAGS += -Wall -Wextra -Werror
CFLAGS += -g

INC =  -Isrc -Isrc/device -Ilua
INC += -I/usr/include/libevdev-1.0/libevdev

LDFLAGS =

LIB = -ldl -llo -lpthread -lmonome -ludev -levdev -lm

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
	events.o \
	gpio.o \
	hid.o \
	lua_eval.o \
	timers.o \
	oracle.o \
	weaver.o \

OBJ = $(patsubst %,$(OBJ_DIR)/%,$(_OBJ))

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	@ mkdir -p $(OBJ_DIR)/device
	$(CC) -c $(CFLAGS) $(INC) $< -o $@  

$(TARGET): $(OBJ) $(STATIC)
	$(LD) $(LDFLAGS) $(OBJ) -o $@ $(STATIC) $(LIB)

lua/liblua.a:
	cd lua && make 

clean:
	rm $(TARGET)
	rm -rf $(OBJ_DIR)
	cd lua && make clean
