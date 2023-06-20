CC = gcc

.c.o:
	gcc -g -c $<

all: oss slave

oss: main.o loglib.o
	$(CC) -Wall -lrt -o oss main.o loglib.o

main.o: main.c log.h pcb.h
	gcc -c main.c

loglib.o: loglib.c log.h
	gcc -c loglib.c

slave: slave.o loglib.o
	$(CC) -Wall -lrt -o slave slave.o loglib.o

slave.o: slave.c log.h pcb.h
	$(CC) -c slave.c

clean:
	rm *.o
