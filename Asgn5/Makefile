CC=gcc
LIBDIR = /usr/local/lib
INCLUDEDIR = /usr/include
FLAGS = -Wall -I.
OBJFILES = mysocket.o queue.o
LIBNAME = libmsocket.a

library: mysocket.o queue.o
	ar rcs $(LIBNAME) mysocket.o queue.o
.PHONY: library

mysocket.o: mysocket.h mysocket.c
	gcc $(FLAGS) -c mysocket.c

queue.o: queue.h queue.c
	gcc $(FLAGS) -c queue.c

install: library
	cp $(LIBNAME) $(LIBDIR)
	cp mysocket.h $(INCLUDEDIR)
	cp queue.h $(INCLUDEDIR)
.PHONY: install
	
clean:
	rm $(OBJFILES) $(LIBNAME)
.PHONY: clean