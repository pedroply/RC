#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#define PORT 58022

int fd;
struct hostent *hostptr;
int addrlen;
char buffer[80];

struct sockaddr_in serveraddr, clientaddr;

int main(){
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(fd == -1)
		perror("Erro ao criar socket");
	char msg[80] = "hi from client";
	char hostName[128];
	if(gethostname(hostName, 128)==-1)
		printf("erro: gethostname\n");
	hostptr = gethostbyname(hostName);


	memset((void*) &serveraddr, (int)'\0', sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = ((struct in_addr*) (hostptr->h_addr_list[0]))->s_addr;
	serveraddr.sin_port = htons((u_short)PORT);

	addrlen = sizeof(serveraddr);

	while(1){
		sendto(fd, msg, strlen(msg),0, (struct sockaddr*) &serveraddr, addrlen);

		recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr*) &serveraddr, &addrlen);

		printf("%s\n", buffer);
		sleep(5);
	}

	close(fd);


	return 0;
}
