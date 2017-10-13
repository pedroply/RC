cs: cs.o client ws
	gcc cs.o -o cs

ws: ws.o
	gcc ws.o -o ws

client: client.o
	gcc client.o -o client

client.o: client.c
	gcc -Wall -g -pedantic -c client.c

ws.o: ws.c
	gcc -Wall -g -pedantic -c ws.c

cs.o: cs.c
	gcc -Wall -g -pedantic -c cs.c
