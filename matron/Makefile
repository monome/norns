TARGET = matron

CC = gcc
LD = gcc

CFLAGS = -g -std=gnu99
LDFLAGS = 
LIB = -llo -llua -lpthread -lSDL2 -lmonome

OBJ_DIR = obj
SRC_DIR = src

_OBJ = main.o \
	args.o \
	events.o \
	m.o \
	repl.o \
	timers.o \
	oracle.o \
	weaver.o

OBJ = $(patsubst %,$(OBJ_DIR)/%,$(_OBJ))

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	$(CC) -c $(CFLAGS) $< -o $@  

$(TARGET): $(OBJ)
	$(LD) $(LDFLAGS) $(OBJ) -o $@ $(LIB)

clean:
	rm $(TARGET)
	rm $(OBJ_DIR)/*.o
