TARGET = ws-wrapper
include ../version.mk

.PHONY = clean

CC = gcc
LD = gcc

CFLAGS += $(VERSION)
CFLAGS += -std=gnu11
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
	@ mkdir -p $(OBJ_DIR)/device
	$(CC) -c $(CFLAGS) $(INC) $< -o $@  


$(TARGET): $(OBJ)
	$(LD) $(LDFLAGS) $(OBJ) -o $@ $(LIB)

clean:
	rm $(TARGET)
	rm -rf $(OBJ_DIR)
