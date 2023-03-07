CC=gcc
LIBDIR = /usr/local/lib
INCLUDEDIR = /usr/include
FLAGS = -Wall -I.
OBJFILES = mysocket.o
LIBNAME = libmsocket.a

library: mysocket.o
	ar rcs $(LIBNAME) mysocket.o
.PHONY: library

mysocket.o: mysocket.h
	gcc $(FLAGS) -c mysocket.c

install: library
	cp $(LIBNAME) $(LIBDIR)
	cp mysocket.h $(INCLUDEDIR)
.PHONY: install
	
clean:
	rm $(OBJFILES) $(LIBNAME)
.PHONY: clean