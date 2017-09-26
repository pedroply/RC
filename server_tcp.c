#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#define PORT 58001

int fd, newfd;
struct hostent *hostptr;
int clientlen;
char buffer[80];
struct sockaddr_in serveraddr, clientaddr;

int main(){
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd == -1)
		perror("Erro ao criar socket");
	char msg[80] = "hi from server";


	memset((void*) &serveraddr, (int)'\0', sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons((u_short)PORT);

	if(bind(fd, (struct sockaddr*) &serveraddr, sizeof(serveraddr)) == -1)
		perror("Error binding socket");

	listen(fd, 5);

	clientlen = sizeof(clientaddr);

	newfd = accept(fd, (struct sockaddr*) &clientaddr, &clientlen);

	while(1){

		read(newfd, &buffer, sizeof(buffer));

		printf("%s\n", buffer);

		write(newfd, msg, sizeof(msg));

		//close(newfd);
	}

	close(fd);


	return 0;
}
