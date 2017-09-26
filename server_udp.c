#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#define PORT 58001

int fd;
struct hostent *hostptr;
int addrlen;
char buffer[80];

struct sockaddr_in serveraddr, clientaddr;

int main(){
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(fd == -1)
		perror("Erro ao criar socket");
	char msg[80] = "hi from server";


	memset((void*) &serveraddr, (int)'\0', sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons((u_short)PORT);

	if(bind(fd, (struct sockaddr*) &serveraddr, sizeof(serveraddr)) == -1)
		perror("Error binding socket");

	addrlen = sizeof(clientaddr);

	while(1){

		recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr*) &clientaddr, &addrlen);

		printf("%s\n", buffer);

		sendto(fd, msg, strlen(msg),0, (struct sockaddr*) &clientaddr, addrlen);
	}

	close(fd);


	return 0;
}
