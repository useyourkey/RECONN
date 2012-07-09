# Makefile for Coolfire Project
#CC=gcc

INCDIR= ./include
CC=arm-none-linux-gnueabi-gcc  -rdynamic -funwind-tables
#CC= gcc -g -D __SIMULATION__
CFLAGS=-I$(INCDIR)

LINTFLAGS:= -Wall -Wextra -Wformat 
#LINTFLAGS:= -Wall 

LIBS=-lpthread -lrt
# All dependencies should be listed here
# to assure they get rebuilt on change
HEADERS=powerMeter.h spectrum.h gps.h reconn.h socket.h dmm.h clientApp.h powerMgmt.h eqptResponse.h gpio.h debugMenu.h reconn_i2c.h fuel_gauge.h libiphoned.h 

H_DEPENDENCIES:=$(addprefix include/, $(HEADERS))

# All object files listed here
OBJ=reconnApp.o gps.o powerMeter.o spectrum.o dmm.o clientApp.o socket.o socketMenu.o powerMgmt.o eqptResponse.o gpio.o crashHandler.o debugMenu.o reconn_i2c.o fuelGauge.o version.o extractBundle.o libiphoned.o clientMenu.o  systemMenu.o fuelGaugeMenu.o dmmMenu.o wifi.o

default: all

Genversion:
	@./GenerateBuildVersion.pl
	$(CC) -c version.c -o version.o $(CFLAGS) $(LINTFLAGS)

all: Genversion reconn-service reconnDaemon

reconnDaemon: powerDaemon.o

# build all objects from all c files.
%.o: %.c $(H_DEPENDENCIES) makefile
	$(CC) -c -o $@ $< $(CFLAGS) $(LINTFLAGS)

powerDaemon.o: powerDaemon.c
	$(CC) $^ -o PowerDaemon $(CFLAGS)
	@cp PowerDaemon ../rootfs/fs/usr/bin

reconn-service: $(OBJ)
	$(CC) -rdynamic -o $@ $^ $(CFLAGS) $(LIBS)
	@rm -f cscope*
	@rm -f tags*
	@ls *.c > cscope.files
	@ls include/*.h >> cscope.files
	@cscope -bkqu -i cscope.files 
	@ctags *c include/*.h
	@rm -f cscope.files
	@cp reconn-service ../rootfs/fs/usr/bin

createBundle: createBundle.c
	gcc -g $< -o $@ $(CFLAGS) $(LINTFLAGS)
	chmod ugo+x $@

.PHONY: reconnSum
reconnSum: reconn-service
	md5sum reconn-service > reconnSum;

.PHONY: upgrade
upgrade: createBundle include/version.h reconn-service reconnSum
	gzip reconn-service
	./createBundle
	gunzip reconn-service.gz 

clean:
	rm -f ./*.o
	rm -f reconn-service
	rm -f createBundle
	rm -f reconnBundle
	rm -f tags
	rm -f cscope*
	rm -f version.c
