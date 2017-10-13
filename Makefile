cs: ws.o client.o cs.o
	gcc -o cs ws.o client.o cs.o -pthread

client.o: client.c client.h
	gcc -Wall -g -pedantic -c client.c

ws.o: ws.c ws.h
	gcc -Wall -g -pedantic -c ws.c

cs.o: cs.c ws.h client.h
	gcc -Wall -g -pedantic -c cs.c