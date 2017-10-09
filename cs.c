#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#define PORT 58022
#define max(A, B) ((A)>=(B)?(A):(B))

int tcpFd, udpFd, newfd, maxfd, counter;
fd_set rfds;
struct hostent *hostptr;
int addrlen, state = 0;
char buffer[80];
struct sockaddr_in serveraddr, clientaddr;

int main(){
	tcpFd = socket(AF_INET, SOCK_STREAM, 0);
  udpFd = socket(AF_INET, SOCK_DGRAM, 0);
	if(tcpFd == -1)
		perror("Erro ao criar socket Tcp");
  if(udpFd == -1)
		perror("Erro ao criar socket Udp");

	char msg[80] = "hi from server";

	memset((void*) &serveraddr, (int)'\0', sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons((u_short)PORT);

	if(bind(tcpFd, (struct sockaddr*) &serveraddr, sizeof(serveraddr)) == -1)
		perror("Error binding socket Tcp");

  if(bind(udpFd, (struct sockaddr*) &serveraddr, sizeof(serveraddr)) == -1)
		perror("Error binding socket Udp");

	listen(tcpFd, 5);

	while(1){
		FD_ZERO(&rfds);
		FD_SET(tcpFd,&rfds);
		maxfd = tcpFd;
		FD_SET(udpFd,&rfds);
		maxfd = max(maxfd,udpFd);


		counter = select(maxfd+1,&rfds,(fd_set*)NULL,(fd_set*)NULL,(struct timeval *)NULL);
		if(counter<=0)
			perror("ERROR: select");


		for(;counter; counter--){
			if(FD_ISSET(tcpFd,&rfds)){
				addrlen = sizeof(clientaddr);
				newfd = accept(tcpFd, (struct sockaddr*) &clientaddr, &addrlen);
				if(newfd == -1){
					perror("ERROR: accept");
				}
				int pid = fork();
				if(pid == 0){ //child
					while(read(newfd, buffer, sizeof(buffer)) == 0);
					//printf("%s, %d\n", buffer, (int)strlen(buffer));
					if(strcmp(buffer, "LST\n") == 0){
						write(newfd, "FPT 4 WCT FLW UPP LOW\n", sizeof("FPT 4 WCT FLW UPP LOW\n"));
					}
					close(newfd);
					return 0;
				}
				else if(pid == -1){ //error
					perror("ERROR: fork");
				}
				else{ //father
					close(newfd);
				}

			}
			else if(FD_ISSET(udpFd,&rfds)){
				recvfrom(udpFd, buffer, sizeof(buffer), 0, (struct sockaddr*) &clientaddr, &addrlen);
				printf("%s\n", buffer);
			}
		}


		/*read(newfd, &buffer, sizeof(buffer));

		printf("TCP: %s\n", buffer);

		write(newfd, msg, sizeof(msg));

    	recvfrom(udpFd, buffer, sizeof(buffer), 0, (struct sockaddr*) &clientaddr, &addrlen);

		printf("UDP: %s\n", buffer);

		sendto(udpFd, msg, strlen(msg),0, (struct sockaddr*) &clientaddr, addrlen);

		printf("loop hehe");*/

	}

  close(newfd);

	close(tcpFd);
	close(udpFd);

	return 0;
}
