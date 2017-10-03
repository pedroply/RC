#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int fd_udp, last_i;
struct hostent *hostptr;
int addrlen;
char buffer[80];
char req[4] = "";
char fileName[40] = "";


struct sockaddr_in serveraddr, clientaddr;

int main(int argc, char** argv){
	fd_udp = socket(AF_INET, SOCK_DGRAM, 0);
	if(fd_udp == -1)
		perror("Erro ao criar socket");
	char msg[80] = "";
	char hostName[128];
	char ws_port_string[1];
	int ws_port = 59000, PORT = 58033;
	int argv_size = (argc - 1) * 3 + (argc - 1);
	char* reg_msg = (char*)malloc(22 + argv_size);
	int j;
	for (j = 1; j < argc ; j++){
		if (!strcmp(argv[j], "WCT") || (!strcmp(argv[j], "LOW")) || (!strcmp(argv[j], "UPP")) || (!strcmp(argv[j], "FLW"))){
			strcat(reg_msg, argv[j]);
			strcat(reg_msg, " ");
		}
		else{
			if (argv[j] != NULL){
				ws_port = atoi(argv[j]);
				printf("%d\n", ws_port);
			}
			if (argv[j+1] != NULL){
				strcpy(hostName, argv[j+1]);
				printf("%s\n", hostName);
			}
			if (argv[j+2] != NULL){
				PORT = atoi(argv[j+2]);
				printf("%d\n", PORT);
			}
			break;
		}
	}
	strcat(reg_msg, "192.107.2.1 ");
	sprintf(ws_port_string, "%d", ws_port);
	strcat(reg_msg, ws_port_string);
	printf("REQ: %s\n", reg_msg);

	if(gethostname(hostName, 128)==-1)
		printf("erro: gethostname\n");
	hostptr = gethostbyname(hostName);


	memset((void*) &serveraddr, (int)'\0', sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = ((struct in_addr*) (hostptr->h_addr_list[0]))->s_addr;
	serveraddr.sin_port = htons((u_short)PORT);

	addrlen = sizeof(serveraddr);

	//sendto(fd_udp)

	while(1){
		//sendto(fd, msg, strlen(msg),0, (struct sockaddr*) &serveraddr, addrlen);
		while(recvfrom(fd_udp, buffer, sizeof(buffer), 0, (struct sockaddr*) &serveraddr, &addrlen) == 0);
		if(!memcmp(buffer, "WRQ ", 4)){
			int i;
			for(i = 4; i < strlen(buffer) && buffer[i] != '\n'; i++){
				if (4 <= i <= 6)
					req[i-4] = buffer[i];
				/*else if ( 8 <= i <= 19){
					fileName[i - 8] = buffer[i];
				}
				else if (i => 21  && buffer[i] != '\0')
					size[i-21] = buffer[i];
					last_i = i;
				else{
					data[i - last_i] = buffer[i];
				}
				 Precisa saber qnt espaco reservar no buffer e no data qnd recebe mensagem*/

			}
			req[3] = '\0';
			if(!strcmp(req, "WCT")){
				//doWordCount(fileName, data);
			}
			else if(!strcmp(req, "FLW")){
				//findLongestWord(fileName, data);
			}
			else if(!strcmp(req, "UPP")){
				//convertUpper(fileName, data);
			}
			else if(!strcmp(req, "LOW")){
				//convertLower(fileName, data);
			}
			else{
				//write ("WRP EOF");
			}
		}
		else{
			// write ("WRP ERR");
		}

		printf("%s\n", buffer);
	}

	close(fd_udp);


	return 0;
}
