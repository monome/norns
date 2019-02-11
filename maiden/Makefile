TARGET = maiden

.PHONY = clean

CC = gcc
LD = gcc

CFLAGS += $(VERSION)
CFLAGS += -std=gnu11
CFLAGS += -Wall -Wextra -Werror
CFLAGS += -g

LDFLAGS = 
LIB = -lpthread -lreadline -lncursesw -lpanel -lnanomsg 

OBJ_DIR = obj
SRC_DIR = src

_OBJ = main.o \
	io.o \
	page.o \
	pages.o \
	ui.o

OBJ = $(patsubst %,$(OBJ_DIR)/%,$(_OBJ))

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	@ mkdir -p $(OBJ_DIR)
	$(CC) -c $(CFLAGS) $< -o $@  

$(TARGET): $(OBJ)
	$(LD) $(LDFLAGS) $(OBJ) -o $@ $(LIB)

clean:
	rm $(TARGET)
	rm -rf $(OBJ_DIR)
