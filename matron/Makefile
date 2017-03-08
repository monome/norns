TARGET = matron

.PHONY = clean

CC = gcc
LD = gcc

CFLAGS = -g -std=gnu11

LDFLAGS = 
LIB = -llo -llua -lpthread -lSDL2 -lmonome
# LIB +=  -lreadline

OBJ_DIR = obj
SRC_DIR = src

_OBJ = main.o \
	args.o \
	events.o \
	input.o \
	lua_eval.o \
	m.o \
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
