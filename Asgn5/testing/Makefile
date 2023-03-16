CC=gcc
FLAGS = -Wall -I

debug: myclient.c myserver.c
	gcc myclient.c -D DEBUG -lmsocket -o client
	gcc myserver.c -D DEBUG -lmsocket -o server
custom: myclient.c myserver.c
	gcc myclient.c -lmsocket -o client
	gcc myserver.c -lmsocket -o server
.PHONY: custom

normal: client.c server.c
	gcc client.c -o client
	gcc server.c -o server
.PHONY: normal

clean:
	rm client server
.PHONY: clean