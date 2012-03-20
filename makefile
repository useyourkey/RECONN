# Makefile for Coolfire Project
#CC=gcc

INCDIR= ./include
CC=arm-none-linux-gnueabi-gcc
#CC= gcc -g -D __SIMULATION__
CFLAGS=-I$(INCDIR)

LINTFLAGS:= -Wall -Wextra -Wformat 
#LINTFLAGS:= -Wall 

LIBS=-lpthread -lrt
# All dependencies should be listed here
# to assure they get rebuilt on change
HEADERS=powerMeter.h spectrum.h gps.h reconn.h socket.h dmm.h clientApp.h powerMgmt.h eqptResponse.h gpio.h

H_DEPENDENCIES:=$(addprefix include/, $(HEADERS))

# All object files listed here
OBJ=reconnApp.o gps.o powerMeter.o spectrum.o dmm.o clientApp.o socket.o powerMgmt.o eqptResponse.o gpio.o  crashHandler.o

all: reconn-service

# build all objects from all c files.
%.o: %.c $(H_DEPENDENCIES) makefile
	$(CC) -c -o $@ $< $(CFLAGS) $(LINTFLAGS)

reconn-service: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)
	@rm -f cscope*
	@rm -f tags*
	@ls *.c > cscope.files
	@ls include/*.h >> cscope.files
	@cscope -bkqu -i cscope.files 
	@ctags *c include/*.h
	@rm -f cscope.files
	@cp reconn-service ../reconn_rootfs/skell/usr/bin
clean:
	rm -f ./*.o
	rm -f reconn-service
