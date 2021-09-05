CC = gcc
CFLAGS=-O2 -Wall
LDLIBS = -lpthread

all: multiclient stockclient stockserver

multiclient: multiclient.c csapp.c csapp.h
stockclient: stockclient.c csapp.c csapp.h
stockserver: stockserver.c echo.c data.c csapp.c csapp.h data.h

clean:
	rm -rf *~ multiclient stockclient stockserver *.o
