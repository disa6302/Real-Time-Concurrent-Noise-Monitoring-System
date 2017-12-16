CC=arm-linux-gnueabihf-gcc

INCLUDES  = -I../inc 
INCLUDES += -I../Unity/src

CFLAGS= -g -O0 $(INCLUDES)



LIBS= -pthread -lrt -lm 


SRCS1 = test_queue.c  ../Unity/src/unity.c main.c  

OBJECTS = $(patsubst %.c,%.o,$(SRCS))

all: queuetest

queuetest:$(OBJECTS)
	$(CC) $(CFLAGS) $(INCLUDES) $(SRCS1) $(LIBS) -o queuetest


.PHONY: clean

clean:
	-rm -f *.o *.NEW *~ *.d
	-rm -f queuetest

