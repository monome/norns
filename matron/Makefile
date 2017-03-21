TARGET = matron

.PHONY = clean

CC = gcc
LD = gcc

CFLAGS = -g -std=gnu11
# CFLAGS += -fstack-protector-all

INC = -Isrc/device -Isrc
INC += -I/usr/include/libevdev-1.0/libevdev

LDFLAGS =

LIB = -llo -llua -lpthread -lmonome -ludev -levdev

OBJ_DIR = obj
SRC_DIR = src

_OBJ = main.o \
	args.o \
	device/device.o \
	device/device_input.o \
	device/device_list.o \
	device/device_monitor.o \
	device/device_monome.o \
	events.o \
	input.o \
	lua_eval.o \
	timers.o \
	oracle.o \
	weaver.o \

OBJ = $(patsubst %,$(OBJ_DIR)/%,$(_OBJ))

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	$(CC) -c $(CFLAGS) $(INC) $< -o $@  

$(TARGET): $(OBJ)
	$(LD) $(LDFLAGS) $(OBJ) -o $@ $(LIB)

clean:
	rm $(TARGET)
	rm $(OBJ_DIR)/*.o
	rm $(OBJ_DIR)/device/*.o
