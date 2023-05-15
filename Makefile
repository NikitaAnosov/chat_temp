cc = gcc
CFLAGS = -g -Wall
PROG = stnc

all: $(PROG)

stnc: stnc.o stnc_params.o server.o client.o
	$(CC) $(FLAGS) -o stnc stnc.o stnc_params.o server.o client.o

%.o: %.c *.h
	$(CC) $(CFLAGS) -c $^

.PHONY: all clean

clean:
	rm -f *.o *.gch $(PROG)

