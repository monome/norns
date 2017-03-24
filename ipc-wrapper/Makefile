TARGET = ipc-wrapper

.PHONY = clean

CC = gcc
LD = gcc

CFLAGS = -std=gnu11
CFLAGS += -Wall -Wextra -Werror
CFLAGS += -g

INC =
LDFLAGS =

LIB = -lpthread -lnanomsg

OBJ_DIR = obj
SRC_DIR = src

_OBJ = main.o


OBJ = $(patsubst %,$(OBJ_DIR)/%,$(_OBJ))

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	$(CC) -c $(CFLAGS) $(INC) $< -o $@  

$(TARGET): $(OBJ)
	$(LD) $(LDFLAGS) $(OBJ) -o $@ $(LIB)

clean:
	rm $(TARGET)
	rm $(OBJ_DIR)/*.o
