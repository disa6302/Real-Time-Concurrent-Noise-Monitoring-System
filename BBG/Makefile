# FileName        : Makefile
# Description     : Generates executables in Client Folder. 
# File Author Name: Bhallaji Venkatesan
# Reference	  : shttp://ecee.colorado.edu/~ecen5623/ecen/ex/Linux/example-1/Makefile 

CC=arm-linux-gnueabihf-gcc
#CC=gcc

CFLAGS= -O0 -g -w -pthread

LIBS = -lrt -lm

OUTPUT=main_exec 

CFILES= main.c

SRCS= ${CFILES}
OBJS= $(CFILES:.c=.o) 

all:  ${OUTPUT}

clean:
	-rm -f *.o *.NEW *~ *.d
	-rm -f ${OUTPUT} ${GARBAGE}

main_exec:main.o
	$(CC) $(CFLAGS) -o $@ main.o $(LIBS)



.c.o:
	$(CC) $(CFLAGS) -c $<

