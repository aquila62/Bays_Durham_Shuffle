OBJ=lfsr.o

CC=gcc

CFLAGS=-c -Wall -O2

LDFLAGS=-lgsl -lgslcblas -lm

lfsr:				$(OBJ)
		$(CC) -Wall -O2 $(OBJ) -o lfsr $(LDFLAGS)

lfsr.o:				lfsr.c
		$(CC) $(CFLAGS) lfsr.c

clean:
		rm -f $(OBJ) lfsr
