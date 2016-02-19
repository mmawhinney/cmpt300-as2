## This is a simple Makefile with lost of comments 
## Check Unix Programming Tools handout for more info.

# Define what compiler to use and the flags.
CC=cc
CXX=CC
CCFLAGS= -g -std=c99 -Wall -Werror -D_XOPEN_SOURCE

all: shell shell

# Compile all .c files into .o files
# % matches all (like * in a command)
# $< is the source file (.c file)
%.o : %.c
	$(CC) -c $(CCFLAGS) $<

# Build shell if necessary
shell: shell.o
	$(CC) -o shell shell.o 

clean:
	rm -f core *.o shell

