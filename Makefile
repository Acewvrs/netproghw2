# Compiler and flags
CC = gcc
CFLAGS = -Wall -g

default: server

# gcc -o server.out lab3_serv.c libunp.a
server:
	$(CC) $(CFLAGS) -o word_guess.out hw2.c libunp.a

# Clean up object files and executables
clean:
	rm -f *.o client server
