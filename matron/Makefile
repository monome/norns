TARGET = matron

CC = gcc
LD = gcc

CFLAGS = -g
LDFLAGS = 
LIB = -lpthread -lSDL2

OBJ_DIR = obj
SRC_DIR = src

_OBJ = main.o events.o timers.o

OBJ = $(patsubst %,$(OBJ_DIR)/%,$(_OBJ))

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	$(CC) -c $(CFLAGS) $< -o $@  

$(TARGET): $(OBJ)
	$(LD) $(LDFLAGS) $(OBJ) -o $@ $(LIB)

clean:
	rm $(TARGET)
	rm $(OBJ_DIR)/*.o
