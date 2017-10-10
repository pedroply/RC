#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#define PORT 58022
#define max(A, B) ((A)>=(B)?(A):(B))

int tcpFd, udpFd, newfd, maxfd, counter, wFd, fileCount = 0;
fd_set rfds;
struct hostent *hostptr;
int addrlen, CSport = PORT;
char buffer[80];
struct sockaddr_in serveraddr, clientaddr, addr;
char hostName[128];
FILE *fileProcessingTasks;

struct filePartitions{
	int fileNumber;
	int filePartitionsLeft;
};

struct filePartitions* fileParts[80] = {NULL};

int main(int argc, char** argv){
	tcpFd = socket(AF_INET, SOCK_STREAM, 0);
	wFd = socket(AF_INET, SOCK_STREAM, 0);
  udpFd = socket(AF_INET, SOCK_DGRAM, 0);
	if(tcpFd == -1)
		perror("Erro ao criar socket Tcp");
  if(udpFd == -1)
		perror("Erro ao criar socket Udp");
	if(wFd == -1)
		perror("Erro ao criar socket Tcp Working Servers");

	for (maxfd = 1; maxfd < argc ; maxfd++){
		if (!strcmp(argv[maxfd], "-p")){
			CSport = atoi(argv[maxfd+1]);
		}
	}

	fileProcessingTasks = (FILE*)fopen("fileprocessingtasks.txt", "w+");
	fclose(fileProcessingTasks);

	if(mkdir("./input_files", 0777) == -1)
	 	perror("ERROR: creating input_files directory"); //n esta a criar diretorio
	if(mkdir("./output_files", 0777) == -1)
	 	perror("ERROR: creating output_files directory"); //n esta a criar diretorio

	char msg[80] = "hi from server";

	memset((void*) &serveraddr, (int)'\0', sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons((u_short)CSport);

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
					while(read(newfd, buffer, sizeof(buffer)-1) == 0);
					buffer[79] = '\0';

					//printf("%s, %d\n", buffer, (int)strlen(buffer));
					if(strcmp(buffer, "LST\n") == 0){
						write(newfd, "FPT 4 WCT FLW UPP LOW\n", sizeof("FPT 4 WCT FLW UPP LOW\n"));
					}
					else if(strncmp(buffer, "REQ ", 4) == 0){ //REQ PTC size data
						char task[4] = "";
						char size[16] = "";
						char taskTemp[4];
						char ipTemp[15];
						char portTemp[6];
						int i, j, charsRead = 0, sizeInt, serversSuported = 0, newLineCount = 0;
						for(i = 4; i<7; i++){
							task[i-4] = buffer[i];
						}
						task[3] = '\0';
						for(i++; buffer[i] != ' '; i++){
							size[i-8] = buffer[i];
						}
						size[i-8] = '\0';
						sizeInt = atoi(size);
						char *fileInBuffer = malloc(sizeof(char)*sizeInt+1);

						for(j = ++i; i<strlen(buffer); i++){
							fileInBuffer[i-j] = buffer[i];
							charsRead++;
						}

						while(charsRead<sizeInt){
							int tempChars = read(newfd, buffer, sizeof(buffer)-1);
							buffer[79] = '\0';
							if(tempChars == -1)
								perror("ERROR: reading rest of file");
							else
								charsRead += tempChars;
							//printf("Read Already: %d; Read Now: %d;\n", charsRead, tempChars);
							strcat(fileInBuffer, buffer);
						}
						/*for(i = 0; strlen(fileInBuffer); i++)
							if(fileInBuffer[i] == '\n')
								newLineCount++;*/

						FILE *fp;
						char directory[80];
						sprintf(directory, "./input_files/%05d.txt", fileCount);
						fp = fopen(directory, "w+");
						if(fp == NULL)
							perror("ERROR: creating file .txt");
						fputs(fileInBuffer, fp);
						fclose(fp);;

						fileCount++;

						//printf("task:%s size:%s\n input: %s\n", task, size, fileInBuffer);

						fileProcessingTasks = (FILE*)fopen("fileprocessingtasks.txt", "r");
						while(fscanf(fileProcessingTasks, "%s %s %s", taskTemp, ipTemp, portTemp) > 0){
							//printf("%s\n", taskTemp);
							if(!strcmp(taskTemp, task)){
								serversSuported++;
							}
						}
						rewind(fileProcessingTasks);
						printf("Suported Servers: %d\n", serversSuported);

						int tempSize = sizeInt/serversSuported;
						int start = 0;
						i = 0;
						while(fscanf(fileProcessingTasks, "%s %s %s", taskTemp, ipTemp, portTemp) > 0){
							//printf("%s\n", taskTemp);

							if(!strcmp(taskTemp, task)){ //dividir e mandar
								if(tempSize + start < sizeInt)
									while(fileInBuffer[tempSize+start] != '\n')
										tempSize++;
								else
									tempSize = sizeInt - start;

								hostptr = gethostbyname(ipTemp);

								memset((void*) &addr, (int)'\0', sizeof(addr));
								addr.sin_family = AF_INET;
								addr.sin_addr.s_addr = ((struct in_addr*) (hostptr->h_addr_list[0]))->s_addr;
								addr.sin_port = htons((u_short)atoi(portTemp));

								if(connect(wFd, (struct sockaddr*) &addr, sizeof(addr)) == -1){
									perror("ERROR: connecting working server tcp");
									return 0;
								}
								char commandHead[80] = "";
								sprintf(commandHead, "WRQ %s %05d%03d %d ", task, fileCount, i, tempSize);  //WRQ PTC filename size data
								printf("sending: %s\n", commandHead);
								if(write(wFd, commandHead, strlen(commandHead)) == -1) // enviar head do comando
									perror("ERROR: write to working server");

								printf("sendind: ");
								write(1, fileInBuffer+start, tempSize);
								printf("\n");

								if(write(wFd, fileInBuffer+start, tempSize) == -1) // enviar file
									perror("ERROR: write to working server");
								close(wFd);
								wFd = socket(AF_INET, SOCK_STREAM, 0);
								if(wFd == -1)
									perror("Erro ao criar socket Tcp Working Servers");
								start = tempSize+1;
								tempSize = sizeInt/serversSuported;
								i++;
							}
						}

						//add to vector containing file partitions

						for(j = 0; j<80; j++){
							if(fileParts[j] == NULL || fileParts[j]->filePartitionsLeft == 0){
								free(fileParts[j]);
								fileParts[j] = malloc(sizeof(struct filePartitions));
								fileParts[j]->fileNumber = fileCount - 1;
								fileParts[j]->filePartitionsLeft = i;
							}
						}



					}
					else if(strncmp(buffer, "REP F ", 6) == 0){ //REP F size data
						char size[16] = "", fileName[9] = "";
						int sizeInt, i, j, charsRead = 0;

						for(i = 6; buffer[i] != ' '; i++){
							size[i-6] = buffer[i];
						}
						size[i-6] = '\0';
						sizeInt = atoi(size);
						for(j = ++i; buffer[i] != ' '; i++){
							fileName[i-j] = buffer[i];
						}
						fileName[i-j] = '\0';

						char *fileInBuffer = malloc(sizeof(char)*sizeInt+1);
						fileInBuffer[0] = '\0';

						for(j = ++i; i < strlen(buffer) && charsRead < sizeInt; i++){
							fileInBuffer[i-j] = buffer[i];
							charsRead++;
						}

						while(charsRead<sizeInt){
							int tempChars = read(newfd, buffer, sizeof(buffer)-1);
							buffer[79] = '\0';
							if(tempChars == -1)
								perror("ERROR: reading rest of file");
							else
								charsRead += tempChars;
							//printf("Read Already: %d; Read Now: %d;\n", charsRead, tempChars);
							strcat(fileInBuffer, buffer);
						}

						FILE *fp;
						char directory[80];
						sprintf(directory, "./output_files/%s", fileName);
						printf("diretorio de file output: %s\n");
						fp = fopen(directory, "w+");
						if(fp == NULL)
							perror("ERROR: creating output file");
						fputs(fileInBuffer, fp);
						fclose(fp);

						/*for(j = 0; j<80; j++){
							if(fileParts[j] != NULL || fileParts[j]->fileNumber == 0){ //vereficar se e a file atual atoi filename dos 5 primeiros digitos
								free(fileParts[j]);
								fileParts[j] = malloc(sizeof(struct filePartitions));
								fileParts[j]->fileNumber = fileCount - 1;
								fileParts[j]->filePartitionsLeft = i;
							}
						}*/




					}
					else if(strncmp(buffer, "REP R ", 6) == 0){ //REP R size data

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
				char port[7] = "";

				for(i = strlen(buffer); i>0; i--){
					if(buffer[i] > 47 && buffer[i] < 58) //numero
						if(buffer[i-1] == ' ') //espaco
							if(buffer[i-2] > 64 && buffer[i-2] < 91){ //letra final de uma task
								j = i;
								break;
							}
				}
				//printf("caractere actual: %c size buffer: %d i: %d\n", buffer[i], strlen(buffer), i);
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
