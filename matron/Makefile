TARGET = matron

.PHONY = clean

CC = gcc
LD = gcc

CFLAGS = -g -std=gnu11
# CFLAGS += -fstack-protector-all

LDFLAGS = 
LIB = -llo -llua -lpthread -lmonome -ludev -levdev

OBJ_DIR = obj
SRC_DIR = src

_OBJ = main.o \
	args.o \
	events.o \
	input.o \
	lua_eval.o \
	m.o \
	timers.o \
	usb_monitor.o \
	oracle.o \
	weaver.o \

OBJ = $(patsubst %,$(OBJ_DIR)/%,$(_OBJ))

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	$(CC) -c $(CFLAGS) $< -o $@  

$(TARGET): $(OBJ)
	$(LD) $(LDFLAGS) $(OBJ) -o $@ $(LIB)

clean:
	rm $(TARGET)
	rm $(OBJ_DIR)/*.o
