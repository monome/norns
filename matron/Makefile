TARGET = matron

CC = gcc
LD = gcc

CFLAGS = -g
LDFLAGS = 
LIB = -llo -llua -lpthread -lSDL2

OBJ_DIR = obj
SRC_DIR = src

_OBJ = main.o \
	events.o \
	event_handle.o \
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
