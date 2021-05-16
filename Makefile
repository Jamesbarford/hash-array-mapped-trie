OUT = build
TARGET = hamt-test.out
CC = cc
CFLAGS = -Wall -Werror -Wextra -Wpedantic -g -O0

$(OUT)/%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

$(OUT)/%.o: ./testing/%.c
	$(CC) -c $(CFLAGS) -o $@ $<

all: $(TARGET)

clean:
	rm $(TARGET)
	rm $(OUT)/*.o

OBJ_LIST = $(OUT)/hamt-testing.o \
           $(OUT)/hamt.o \
           $(OUT)/print_bits.o

$(TARGET): $(OBJ_LIST)
	$(CC) -o $(TARGET) $(OBJ_LIST)

$(OUT)/hamt-testing.o: ./hamt-testing.c ./testing/print_bits.h
$(OUT)/hamt.o: ./hamt.c ./hamt.h
$(OUT)/print_bits.o: ./testing/print_bits.c ./testing/print_bits.h
