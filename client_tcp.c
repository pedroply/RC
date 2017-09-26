#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#define PORT 58022

int fd, newfd;
struct hostent *hostptr;
int clientlen;
char buffer[80];
struct sockaddr_in addr, clientaddr;

int main(){
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd == -1)
		perror("Erro ao criar socket");
	char msg[80] = "hi from client";
	char hostName[128];
	if(gethostname(hostName, 128)==-1)
		printf("erro: gethostname\n");
	hostptr = gethostbyname(hostName);


	memset((void*) &addr, (int)'\0', sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ((struct in_addr*) (hostptr->h_addr_list[0]))->s_addr;
	addr.sin_port = htons((u_short)PORT);

	if(connect(fd, (struct sockaddr*) &addr, sizeof(addr)) == -1)
		printf("erro: connect");

	while(1){
		write(fd, msg, sizeof(msg));

		read(fd, &buffer, sizeof(buffer));

		printf("%s\n", buffer);

		sleep(5);
	}
	close(fd);

	return 0;
}
