OUT = build
TARGET = testing.out
CC = cc
CFLAGS = -Wall -Werror -Wextra -Wpedantic -g -O0

$(OUT)/%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

all: $(TARGET)

clean:
	rm $(TARGET)
	rm $(OUT)/*.o

OBJ_LIST = $(OUT)/hamt-testing.o\
           $(OUT)/hamt.o

$(TARGET): $(OBJ_LIST)
	$(CC) -o $(TARGET) $(OBJ_LIST)

$(OUT)/hamt-testing.o: ./hamt-testing.c
$(OUT)/hamt.o: ./hamt.c ./hamt.h
