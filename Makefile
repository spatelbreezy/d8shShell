# Provide a makefile with a target called shell_jr that 
# creates the executable shell_jr.  Also provide a clean
# target the removes the executable and any .o file(s).
#

CC = gcc
CFLAGS = -ansi -Wall -g -O0 -Wwrite-strings -Wshadow -pedantic-errors \
-fstack-protector-all -Wextra
PROGS = d8sh

.PHONY: all clean 

all: $(PROGS)

clean:
	rm -f *.o $(PROGS) a.out

d8sh: d8sh.o executor.o lexer.o parser.tab.o
	$(CC) -lreadline -o d8sh d8sh.o lexer.o parser.tab.o executor.o

d8sh.o: d8sh.c executor.h lexer.h
	$(CC) $(CFLAGS) -c d8sh.c

executor.o: executor.c command.h executor.h
	$(CC) $(CFLAGS) -c executor.c

lexer.o: lexer.c parser.tab.h
	$(CC) $(CFLAGS) -c lexer.c

parser.tab.o: parser.tab.c command.h
	$(CC) $(CFLAGS) -c parser.tab.c




