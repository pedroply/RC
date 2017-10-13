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
#include <sys/wait.h>
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

void childHandler(int sig)
{
  pid_t pid;
	int status;
  //pid = wait(&status);
  pid =  waitpid(-1, &status, WUNTRACED);
	WEXITSTATUS(status);
	printf("-----status child: %d\n", status);
	if(status == 512)
		fileCount++;
}

int main(int argc, char** argv){
	tcpFd = socket(AF_INET, SOCK_STREAM, 0);
  udpFd = socket(AF_INET, SOCK_DGRAM, 0);
	if(tcpFd == -1)
		perror("Erro ao criar socket Tcp");
  if(udpFd == -1)
		perror("Erro ao criar socket Udp");
	if(wFd == -1)
		perror("Erro ao criar socket Tcp Working Servers");

	signal(SIGCHLD, childHandler);

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

	addrlen = sizeof(clientaddr);

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
						char *supportedTasks[99], tempTask[4], tempIp[15], tempPort[7], sendLst[80] = "";
						int supportedTasksCount = 0, i;
						fileProcessingTasks = (FILE*)fopen("fileprocessingtasks.txt", "r");
						for(i = 0; i<99; i++){
							supportedTasks[i] = malloc(4);
							supportedTasks[i][0] = '\0';
						}
						while(fscanf(fileProcessingTasks, "%s %s %s", tempTask, tempIp, tempPort) > 0){
							int a;
							int b = 0;
							for(a = 0; a<supportedTasksCount; a++){
								if(supportedTasks[a][0] != '\0' && !strcmp(supportedTasks[a], tempTask)){
									b = 1;
								}
							}
							for(a = 0; supportedTasks[a][0] != '\0'; a++);
							if(!b){
								strcpy(supportedTasks[a], tempTask);
								supportedTasksCount++;
								//printf("added: %s at: %d\n", tempTask, a);
							}
						}
						fclose(fileProcessingTasks);

						sprintf(sendLst, "FPT %d", supportedTasksCount);
						for(i = 0; i<supportedTasksCount; i++){
							strcat(sendLst, " ");
							strcat(sendLst, supportedTasks[i]);
						}
						strcat(sendLst, "\n");
						if(supportedTasksCount>0)
							write(newfd, sendLst, strlen(sendLst));
						else
							write(newfd, "FPT EOF\n", strlen("FPT EOF\n"));
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

						fileCount++; //tem de filho mandar ao cs principal se criou novo ficheiro de output

						FILE *fp;
						char directory[80];
						sprintf(directory, "./input_files/%05d.txt", fileCount);
						fp = fopen(directory, "w+");
						if(fp == NULL)
							perror("ERROR: creating file .txt");
						fputs(fileInBuffer, fp);
						fclose(fp);;


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
								if(tempSize + start < sizeInt){
									while(fileInBuffer[tempSize+start] != '\n')
										tempSize++;
									tempSize++;
									}
								else
									tempSize = sizeInt - start;

								hostptr = gethostbyname(ipTemp);

								memset((void*) &addr, (int)'\0', sizeof(addr));
								addr.sin_family = AF_INET;
								addr.sin_addr.s_addr = ((struct in_addr*) (hostptr->h_addr_list[0]))->s_addr;
								addr.sin_port = htons((u_short)atoi(portTemp));

								int sendindChild = fork();

								if(sendindChild == 0){ //codigo sendingChild
									wFd = socket(AF_INET, SOCK_STREAM, 0);
									if(connect(wFd, (struct sockaddr*) &addr, sizeof(addr)) == -1){
										perror("ERROR: connecting working server tcp");
										return 0;
									}
									char commandHead[80] = "";
									char fileName[13] = "";
									sprintf(fileName, "%05d%03d.txt", fileCount, i);
									sprintf(commandHead, "WRQ %s %05d%03d.txt %d ", task, fileCount, i, tempSize);  //WRQ PTC filename size data
									printf("sending: %s\n", commandHead);
									if(write(wFd, commandHead, strlen(commandHead)) == -1) // enviar head do comando
										perror("ERROR: write to working server");

									printf("sendind: %s\nstart: %d, tempsize: %d\n", fileInBuffer, start, tempSize);
									write(1, fileInBuffer+start, tempSize);
									printf("\n");

									if(write(wFd, fileInBuffer+start, tempSize) == -1) // enviar file
										perror("ERROR: write to working server");

									//falta ficar a espera de receber e guardar no ficheiro
									/*FD_ZERO(&rfds); //n e preciso devido ao while acho eu
									FD_SET(wFd,&rfds);
									maxfd = tcpFd;
									counter = select(maxfd+1,&rfds,(fd_set*)NULL,(fd_set*)NULL,(struct timeval *)NULL);
									if(counter<=0)
										perror("ERROR: select");*/

									printf("waiting for response\n");

									i = read(wFd, buffer, sizeof(buffer)-1);
									while(i == 0){
										i = read(wFd, buffer, sizeof(buffer)-1);
									}
									printf("i: %d\n", i);
									buffer[i] = '\0';

									printf("buffer: %s\n", buffer);

									if(strncmp(buffer, "REP F ", 6) == 0){
										for(i = 6; buffer[i] != ' '; i++){
											size[i-6] = buffer[i];
										}
										size[i-6] = '\0';
										sizeInt = atoi(size);
										free(fileInBuffer);
										charsRead = 0;
										fileInBuffer = malloc(sizeof(char)*sizeInt+1);
										fileInBuffer[0] = '\0';

										//printf("size: %s\nbuffer: %s\nbufferLen: %d\n", size, buffer, (int)strlen(buffer));
										i++;
										for(j = i; i < strlen(buffer) && charsRead < sizeInt; i++){
											fileInBuffer[i-j] = buffer[i];
											charsRead++;
											printf("%c %d", buffer[i], i);
										}
										fileInBuffer[i-j] = '\0';
										//printf("to while falta ler%d\n", sizeInt-charsRead);

										while(charsRead<sizeInt-1){ //esta a mandar menos 1?? mario
											int tempChars = read(wFd, buffer, sizeof(buffer)-1);
											buffer[tempChars] = '\0';
											if(tempChars == -1)
												perror("ERROR: reading rest of file");
											else
												charsRead += tempChars;
											//printf("Read Already: %d; Read Now: %d;\n", charsRead, tempChars);
											strcat(fileInBuffer, buffer);
											printf("buffer: %s\n", buffer);
										}

										printf("%s\n", fileInBuffer);

										FILE *fp;
										char directory[80];
										sprintf(directory, "./output_files/%s", fileName);
										printf("diretorio de file output: %s\n", fileName);
										fp = fopen(directory, "w+");
										if(fp == NULL)
											perror("ERROR: creating output file");
										fputs(fileInBuffer, fp);
										fclose(fp);
									}
									else if(strncmp(buffer, "REP R ", 6) == 0){ //REP R size data
										for(i = 6; buffer[i] != ' '; i++){
											size[i-6] = buffer[i];
										}
										size[i-6] = '\0';
										sizeInt = atoi(size);
										free(fileInBuffer);
										charsRead = 0;
										fileInBuffer = malloc(sizeof(char)*sizeInt+1);
										fileInBuffer[0] = '\0';
										i++;
										for(j = i; i < strlen(buffer) && charsRead < sizeInt; i++){
											fileInBuffer[i-j] = buffer[i];
											charsRead++;
											printf("%c %d", buffer[i], i);
										}
										fileInBuffer[i-j] = '\0';
										while(charsRead<sizeInt-1){ //esta a mandar menos 1?? mario
											int tempChars = read(wFd, buffer, sizeof(buffer)-1);
											buffer[tempChars] = '\0';
											if(tempChars == -1)
												perror("ERROR: reading rest of file");
											else
												charsRead += tempChars;
											//printf("Read Already: %d; Read Now: %d;\n", charsRead, tempChars);
											strcat(fileInBuffer, buffer);
											printf("buffer: %s\n", buffer);
										}
										printf("%s\n", fileInBuffer);
										FILE *fp;
										char directory[80];
										sprintf(directory, "./output_files/%s", fileName);
										printf("diretorio de file output: %s\n", fileName);
										fp = fopen(directory, "w+");
										if(fp == NULL)
											perror("ERROR: creating output file");
										fputs(fileInBuffer, fp);
										fclose(fp);
									}
									else{
										perror("ERROR: mensagem rep ws mal formatada");
									}

									close(wFd);
									return 0;
								}
								else if(sendindChild == -1){
									perror("ERROR: fork sendingChild");
								}
								else{ //codigo pai
									close(wFd);
									wFd = socket(AF_INET, SOCK_STREAM, 0);
									if(wFd == -1)
										perror("Erro ao criar socket Tcp Working Servers");
									start += tempSize;
									tempSize = sizeInt/serversSuported;
									i++;
								}
							}
						}

						//wait for childs to get processed files then make final and send to user

						waitpid(-1, NULL, 0);

						printf("All files processed should be in output_files\n");

						//i num de sub files
						//sizeInt num de caracteres sem \0
						//fileCount numero de files processadas sendo a filecount a atual
						if ((!strcmp(task, "UPP")) || (!strcmp(task, "LOW"))){
							fileInBuffer[0] = '\0';
							for(j = 0; j<i; j++){
								FILE *fp;
								char directory[80];
								sprintf(directory, "./output_files/%05d%03d.txt", fileCount, j);
								fp = fopen(directory, "r");
								if(fp == NULL)
									perror("ERROR: reading one of the output files");
								char c, *tempString;
								tempString = malloc(sizeInt+1);
								tempString[0] = '\0';
								int k;
								for(k = 0; (c = fgetc(fp)) != EOF; k++) {
					        		tempString[k] = c;
						   		}
								tempString[k] = '\0';
								strcat(fileInBuffer, tempString);
								fclose(fp);
							}

							//fileInBuffer has the complete processed file
							//has to responde with result to the user

							//REP R/F size data

							//char response[sizeInt+16+4+2+2] = "REP F ";
							char *response = malloc(sizeInt+16+4+2+2);
							response[0] = '\0';
							strcat(response, "REP F ");
							strcat(response, size);
							strcat(response, " ");
							strcat(response, fileInBuffer);
							write(newfd, response, strlen(response));
							close(newfd);
							return 2;
						}
						else if (!strcmp(task, "WCT")){
							fileInBuffer[0] = '\0';
							char* processed_responses[serversSuported];
							char size_WCT[16];
							int l = 0, count = 0;
							for (i = 0; i < serversSuported; i++){
								processed_responses[i] = malloc(16);
								processed_responses[i][0] = '\0';
							}
							for(j = 0; j<i; j++){
								FILE *fp;
								char directory[80];
								sprintf(directory, "./output_files/%05d%03d.txt", fileCount, j);
								fp = fopen(directory, "r");
								if(fp == NULL)
									perror("ERROR: reading one of the output files");
								char c, *tempString;
								tempString = malloc(sizeInt+1);
								tempString[0] = '\0';
								int k;
								for(k = 0; (c = fgetc(fp)) != EOF; k++) {
					        		tempString[k] = c;
					        		processed_responses[l][k] = tempString[k];
						    	}
								tempString[k] = '\0';
								processed_responses[l][k] = '\0';
								l++;
								fclose(fp);
							}
							for (i = 0; i < l; i++){
								count += atoi(processed_responses[i]);
							}
							sprintf(size_WCT, "%d", count);
							char *response = malloc(sizeInt+16+4+2+2);
							response[0] = '\0';
							strcat(response, "REP R ");
							strcat(response, size_WCT);
							strcat(response, " ");
							strcat(response, fileInBuffer);
							write(newfd, response, strlen(response));
							close(newfd);
							return 2;
						}
						else if (!strcmp(task, "FLW")){
							fileInBuffer[0] = '\0';
							for(j = 0; j<i; j++){
								FILE *fp;
								char directory[80];
								sprintf(directory, "./output_files/%05d%03d.txt", fileCount, j);
								fp = fopen(directory, "r");
								if(fp == NULL)
									perror("ERROR: reading one of the output files");
								char c, *tempString;
								tempString = malloc(sizeInt+1);
								tempString[0] = '\0';
								int k;
								for(k = 0; (c = fgetc(fp)) != EOF; k++) {
					        		tempString[k] = c;
						    	}
								tempString[k] = '\0';
								strcat(fileInBuffer, tempString);
								fclose(fp);
							}
							char *response = malloc(sizeInt+16+4+2+2);
							response[0] = '\0';
							strcat(response, "REP R ");
							strcat(response, size);
							strcat(response, " ");
							strcat(response, fileInBuffer);
							write(newfd, response, strlen(response));
							close(newfd);
							return 2;
						}
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

				int readBytes = recvfrom(udpFd, buffer, sizeof(buffer), 0, (struct sockaddr*) &clientaddr, &addrlen);  //REG WCT UPP 127.0.1.1 59000
				buffer[readBytes] = '\0';
				int i, j = -1, test, charsInFile = 0;
				char ip[15] = "", tempIp[15] = "";
				char port[7] = "", tempPort[7] = "";
				char tempTask[4] = "", nextChar, *newFileServer;

				printf("%s\n", buffer);
				if(!strncmp(buffer, "UNR ", 4)){ //unregister server

					for(i = strlen(buffer); i>0; i--){
						if(buffer[i] > 47 && buffer[i] < 58) //numero
							if(buffer[i-1] == ' ') //espaco
								if(buffer[i-2] > 64 && buffer[i-2] < 91){ //letra final de uma task
									j = i;
									break;
								}
					}
					//printf("caractere actual: %c size buffer: %d i: %d\n", buffer[i], strlen(buffer), i);
					if(j == -1){
						printf("mensagem do ws mal formulada\n");
						if(sendto(udpFd, "UAK NOK\n", (int)strlen("UAK NOK\n"),0, (struct sockaddr*) &clientaddr, addrlen) == -1)
							perror("Error sending unregister nok message");
						break;
					}
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

					//check for repeated ip and port combo      FAZER UNREGISTER
					fileProcessingTasks = (FILE*)fopen("fileprocessingtasks.txt", "r");
					test = 0;
					nextChar = getc(fileProcessingTasks);
					while(nextChar != EOF){
						charsInFile++;
						nextChar = getc(fileProcessingTasks);
					}
					rewind(fileProcessingTasks);
					newFileServer = malloc(charsInFile);
					newFileServer[0] = '\0';
					while(fscanf(fileProcessingTasks, "%s %s %s", tempTask, tempIp, tempPort) > 0){
						//printf("%s == %s && %d == %d\n", ip, tempIp,  atoi(port), atoi(tempPort));
						if(!(!strcmp(ip, tempIp) && atoi(port) == atoi(tempPort))){ //se n for do servidar a desregistar adicionar
							strcat(newFileServer, tempTask);
							strcat(newFileServer, " ");
							strcat(newFileServer, tempIp);
							strcat(newFileServer, " ");
							strcat(newFileServer, tempPort);
							strcat(newFileServer, "\n");
						}
						test = 1;
					}
					fclose(fileProcessingTasks);
					fileProcessingTasks = (FILE*)fopen("fileprocessingtasks.txt", "w+");
					fputs(newFileServer, fileProcessingTasks);
					fclose(fileProcessingTasks);

					if(!test){ //n estava registado
						if(sendto(udpFd, "UAK NOK\n", (int)strlen("UAK NOK\n"),0, (struct sockaddr*) &clientaddr, addrlen) == -1)
							perror("Error sending unregister nok message");
						break;
					}

					if(sendto(udpFd, "UAK OK\n", (int)strlen("UAK OK\n"),0, (struct sockaddr*) &clientaddr, addrlen) == -1)
						perror("Error sending unregister ok message");
					break;
				}


				//register server and verify message type?

				if(strncmp(buffer, "REG ", 4) != 0){
					printf("mensagem reg ws desconhecida\n");
					if(sendto(udpFd, "RAK NOK\n", (int)strlen("RAK NOK\n"),0, (struct sockaddr*) &clientaddr, addrlen) == -1)
						perror("Error sending register message");
					break;
				}

				for(i = strlen(buffer); i>0; i--){
					if(buffer[i] > 47 && buffer[i] < 58) //numero
						if(buffer[i-1] == ' ') //espaco
							if(buffer[i-2] > 64 && buffer[i-2] < 91){ //letra final de uma task
								j = i;
								break;
							}
				}
				//printf("caractere actual: %c size buffer: %d i: %d\n", buffer[i], strlen(buffer), i);
				if(j == -1){
					printf("mensagem do ws mal formulada\n");
					if(sendto(udpFd, "RAK NOK\n", (int)strlen("RAK NOK\n"),0, (struct sockaddr*) &clientaddr, addrlen) == -1)
						perror("Error sending register message");
					break;
				}
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

				//check for repeated ip and port combo
				fileProcessingTasks = (FILE*)fopen("fileprocessingtasks.txt", "r");
				test = 0;
				while(fscanf(fileProcessingTasks, "%s %s %s", tempTask, tempIp, tempPort) > 0){
					//printf("%s == %s && %d == %d\n", ip, tempIp,  atoi(port), atoi(tempPort));
					if(!strcmp(ip, tempIp) && atoi(port) == atoi(tempPort)){
						test = 1;
						break;
					}
				}
				if(test){
					printf("servidor ws ja registado\n");
					if(sendto(udpFd, "RAK ERR\n", (int)strlen("RAK ERR\n"),0, (struct sockaddr*) &clientaddr, addrlen) == -1)
						perror("Error sending register message");
					break;
				}
				fclose(fileProcessingTasks);

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

						/*int a;
						int b = 0;
						for(a = 0; a<99; a++){
							if(supportedTasks[a] != NULL && !strcmp(supportedTasks[a], task)){
								b = 1
							}
						}
						for(a = 0; supportedTasks[a] != NULL; a++);
						if(!b)
							strcpy(supportedTasks[a], task);*/

						//printf("%s %s %s\n", task, ip, port);
						fprintf(fileProcessingTasks, "%s %s %s", task, ip, port);
					}
				}
				//printf("%s\n", buffer);
				fclose(fileProcessingTasks);
				sendto(udpFd, "RAK OK\n", strlen("RAK OK\n"),0, (struct sockaddr*) &clientaddr, addrlen);
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
