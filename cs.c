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

int tcpFd, udpFd, newfd, maxfd, counter, wFd;
fd_set rfds;
struct hostent *hostptr;
int addrlen, state = 0;
char buffer[80];
struct sockaddr_in serveraddr, clientaddr, addr;
char hostName[128];
FILE *fileProcessingTasks;

int main(){
	tcpFd = socket(AF_INET, SOCK_STREAM, 0);
	wFd = socket(AF_INET, SOCK_STREAM, 0);
  udpFd = socket(AF_INET, SOCK_DGRAM, 0);
	if(tcpFd == -1)
		perror("Erro ao criar socket Tcp");
  if(udpFd == -1)
		perror("Erro ao criar socket Udp");
	if(wFd == -1)
		perror("Erro ao criar socket Tcp Working Servers");

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
					else if(strncmp(buffer, "REQ ", 4) == 0){ //REQ PTC size data
						/*char task[4] = "";
						char size[16] = "";
						int i;
						for(i = 4; i<7; i++){
							task[i-4] = buffer[i];
						}
						task[3] = '\0';
						for(i++; buffer[i] != ' '; i++){
							size[i-8] = buffer[i];
						}
						size[i] = '\0';

						printf("task:%s size:%s\n", task, size);*/


						if(gethostname(hostName, 128)==-1){
							printf("erro: gethostname\n");
							return 0;
						}
						hostptr = gethostbyname(hostName);


						memset((void*) &addr, (int)'\0', sizeof(addr));
						addr.sin_family = AF_INET;
						addr.sin_addr.s_addr = ((struct in_addr*) (hostptr->h_addr_list[0]))->s_addr;
						addr.sin_port = htons((u_short)59000);

						if(connect(wFd, (struct sockaddr*) &addr, sizeof(addr)) == -1){
							perror("ERROR: connecting working server tcp");
							return 0;
						}

						if(write(wFd, "WRQ UPP 12345678.txt 102462524 ola o mario e super gay ehehehe", sizeof("WRQ UPP 12345678.txt 102462524 ola o mario e super gay ehehehe")) == -1)
							perror("ERROR: write to working server");

						close(wFd);
						wFd = socket(AF_INET, SOCK_STREAM, 0);
						if(wFd == -1)
							perror("Erro ao criar socket Tcp Working Servers");


					}
					else
						write(newfd, "REQ ERR\n", sizeof("REQ ERR\n"));
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
				recvfrom(udpFd, buffer, sizeof(buffer), 0, (struct sockaddr*) &clientaddr, &addrlen);  //REG WCT UPP 127.0.1.1 59000
				int i, j = -1;
				char ip[15] = "";
				char port[5] = "";

				for(i = strlen(buffer); i>0; i--){
					if(buffer[i] > 47 && buffer[i] < 58) //numero
						if(buffer[i-1] == ' ') //espaco
							if(buffer[i-2] > 64 && buffer[i-2] < 91){ //letra final de uma task
								j = i;
								break;
							}
				}
				if(j == -1)
					printf("mensagem do ws mal formulada");
				for(j = 0; i<strlen(buffer) && buffer[i] != ' '; i++){
					ip[j] = buffer[i];
					j++;
				}
				ip[j] = '\0';
				i++;
				for(j = 0; i<strlen(buffer) && buffer[i] != ' '; i++){
					port[j] = buffer[i];
					j++;
				}
				port[j] = '\0';

				//printf("ip: %s port: %s\n", ip, port);
				fileProcessingTasks = (FILE*)fopen("fileprocessingtasks.txt", "a");
				for(i = 0; i<80 && i<strlen(buffer); i++){
					if(i<4 && buffer[i]!="REG "[i])
						break;
					if(i>3 && buffer[i]!=' ' && buffer[i] > 64 && buffer[i] < 91){
						char task[4] = "";
						for(j = i+3; i<j; i++){
							task[i+3-j] = buffer[i];
						}
						task[3] = '\0';
						//printf("%s %s %s\n", task, ip, port);
						fprintf(fileProcessingTasks, "%s %s %s", task, ip, port);
					}
				}
				//printf("%s\n", buffer);
				fclose(fileProcessingTasks);
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
