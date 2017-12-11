

# FileName        :Makefile
# Description     :Generates executables in ~/APES_Beaglebone_linux Takes .h files from ~/APES_Beaglebone_linux_project/inc
#		   path
# File Author Name:Divya Sampath Kumar, Bhallaji Venkatesan
# Reference	  :http://ecee.colorado.edu/~ecen5623/ecen/ex/Linux/example-1/Makefile 



INCLUDE_DIRS = -Iinc
CC=arm-linux-gnueabihf-gcc
#CC=gcc
CFLAGS= -O0 -g -w $(INCLUDE_DIRS) -pthread
LIBS =  -lrt -lm
OUTPUT=main_exec
#OUTPUT=i2c_light.o

#Will be added as we add on header and C files
HFILES= message.h decision_led.h
CFILES= src/main.c 
#CFILES= src/main.c src/i2c-drive.c


SRCS = $(wildcard src/*.c)
OBJECTS = $(patsubst %.c,%.o,$(SRCS))

all:	${OUTPUT}

#OUTPUT to be filled later on
led.o:$(OBJECTS)
	$(CC) $(CFLAGS) -o $@ led.o $(LIBS)

main_exec:$(OBJECTS)
	$(CC) $(CFLAGS) -o $@ main.o led.o $(LIBS)

#i2c_drive:$(OBJECTS)
#	$(CC) $(CFLAGS) -o $@ i2c-drive.o $(LIBS)

clean:
	-rm -f *.o *.NEW *~ *.d
	-rm -f ${OUTPUT} ${GARBAGE}

$(OBJECTS):src/%.o : src/%.c
	$(CC) $(CFLAGS) -c $< 
