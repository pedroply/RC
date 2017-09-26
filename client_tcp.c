#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#define PORT 58011

int fd, newfd;
struct hostent *hostptr;
int clientlen;
char buffer[80];
struct sockaddr_in addr, clientaddr;

int main(){
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd == -1)
		perror("Erro ao criar socket");
	char msg[80] = "";
	char hostName[128];
	char req[4] = "";
	char fileName[40] = "";
	if(gethostname(hostName, 128)==-1){
		printf("erro: gethostname\n");
		return 0;
	}
	hostptr = gethostbyname("tejo");


	memset((void*) &addr, (int)'\0', sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ((struct in_addr*) (hostptr->h_addr_list[0]))->s_addr;
	addr.sin_port = htons((u_short)PORT);

	if(connect(fd, (struct sockaddr*) &addr, sizeof(addr)) == -1){
		printf("erro: connect");
		return 0;
	}

	while(1){
		fgets(msg, 80, stdin);
		printf("%s\n", msg);
		if(!strcmp(msg, "list\n")){
			write(fd, "LST\n", sizeof(msg));
			read(fd, &buffer, sizeof(buffer));
			printf("%s\n", buffer);
		}
		else if(!strcmp(msg, "exit\n")){
			close(fd);
			return 0;
		}
		else{
			int i;
			for(i = 0; i<strlen(msg); i++){
				if(i<8 && msg[i]!="request "[i])
					break;
				if(i>7 && i<11)
					req[i-8] = msg[i];
				if(i>11)
					fileName[i-12] = msg[i];
				printf("%d", i);
			}
			req[3] = '\0';
			fileName[i] = '\0';

			printf("\n%d %s %s\n", i, req, fileName);
		}


		/*if (!fgets(msg, 80, stdin))
		    printf("erro: command");

		printf("%s\n", msg);

		write(fd, msg, sizeof(msg));

		read(fd, &buffer, sizeof(buffer));

		printf("%s\n", buffer);*/
	}
	close(fd);

	return 0;
}
