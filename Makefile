CC=gcc
CFLAGS=-O2 -Wall -g -ggdb
LDFLAGS=

OBJECTS=main.o   pretty.o algorithm.o generate.o
HEADERS=basics.h pretty.h algorithm.h generate.h
TARGET=sudoku-solver

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

run: $(TARGET)
	./$(TARGET) extreme1.txt

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGET)
