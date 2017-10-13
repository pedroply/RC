#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#define PORT 58022

int fd, newfd, CSport = PORT;
struct hostent *hostptr;
int clientlen;
char buffer[80];
struct sockaddr_in addr, clientaddr;
char hostName[128] = "";

int main(int argc, char** argv){
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd == -1)
		perror("Erro ao criar socket");

	if(gethostname(hostName, 128)==-1){
		printf("erro: gethostname\n");
		return 0;
	}

	for (newfd = 1; newfd < argc ; newfd++){
		if (!strcmp(argv[newfd], "-p")){
				CSport = atoi(argv[newfd+1]);
		}
		else if (!strcmp(argv[newfd], "-n")){
				strcpy(hostName, argv[newfd+1]);
		}
	}

	struct sigaction new_actn, old_actn;
	new_actn.sa_handler = SIG_IGN;
	sigemptyset (&new_actn.sa_mask);
	new_actn.sa_flags = 0;
	sigaction (SIGPIPE, &new_actn, &old_actn);
	char msg[80] = "";
	char req[4] = "";
	char fileName[40] = "";
	hostptr = gethostbyname(hostName);


	memset((void*) &addr, (int)'\0', sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ((struct in_addr*) (hostptr->h_addr_list[0]))->s_addr;
	addr.sin_port = htons((u_short)PORT);

	if(connect(fd, (struct sockaddr*) &addr, sizeof(addr)) == -1){
		printf("erro: connect");
		return 0;
	}

	while(1){
		int charsRead = 0;
		printf(": ");
		fgets(msg, 80, stdin);
		if(!strcmp(msg, "list\n")){
			if(write(fd, "LST\n", 5) == -1)
				printf("Error: write to socket");
			while((charsRead = read(fd, buffer, sizeof(buffer))) == 0);
			buffer[charsRead] = '\0';
			if(!strcmp("FPT EOF\n", buffer)){
				printf("\tNo FPTs available!\n");
			}
			int i;
			char numFTP[3] = " ";
			for(i = 4; i<strlen(buffer) && buffer[i]!=' '; i++)
				numFTP[i-4] = buffer[i];
			numFTP[i-4] = '\0';
			int inumFTP = atoi(numFTP);
			for(i = 0; i<inumFTP; i++){
				char cm[4] = "";
				char description[80] = "";
				int j;
				for(j = 6 + i*4; j<strlen(buffer) && buffer[j]!=' '; j++)
					cm[j-6-i*4] = buffer[j];
				cm[3] = '\0';
				if(!strcmp(cm, "WCT"))
					strcpy(description, "word count");
				else if(!strcmp(cm, "FLW"))
					strcpy(description, "find longest word");
				else if(!strcmp(cm, "UPP"))
					strcpy(description, "convert text to upper case");
				else if(!strcmp(cm, "LOW"))
					strcpy(description, "convert text to lower case");
				else
					strcpy(description, "no suitable description");
				printf("\t %d- %s - %s\n", i+1, cm, description);
			}
		}
		else if(!strcmp(msg, "exit\n")){
			close(fd);
			return 0;
		}
		else if(!memcmp(msg, "request ", 8)){

			int i, charsInFile = 1;
			FILE *inFile;
			char inbuffer[1024] = "", nextChar;
			for(i = 0; i<strlen(msg) && msg[i]!='\n'; i++){
				if(i<8 && msg[i]!="request "[i])
					break;
				if(i>7 && i<11)
					req[i-8] = msg[i];
				if(i>11)
					fileName[i-12] = msg[i];
			}
			req[3] = '\0';
			fileName[i-12] = '\0';

			inFile = (FILE*)fopen(fileName, "r");
			nextChar = getc(inFile);
			while(nextChar != EOF){
				charsInFile++;
				nextChar = getc(inFile);
			}

			char sizeFile[20] = "";
			sprintf(sizeFile, " %d ", charsInFile);

			int charsize = strlen(sizeFile);
			int reqSize = 8+charsize+charsInFile;
			char* sendReq = (char*)malloc(reqSize);
			char* recvReq = (char*)malloc(reqSize);
			sendReq[0] = '\0';
			recvReq[0] = '\0';
			strcat(sendReq, "REQ ");
			strcat(sendReq, req);
			strcat(sendReq, sizeFile);

			rewind(inFile);
			while(fgets(inbuffer, 1024, (FILE*)inFile) != NULL)
				strcat(sendReq, inbuffer);

			//printf("%s |size: %d\n", sendReq, reqSize);
	    if(write(fd, sendReq, reqSize) == -1)
				printf("Error: write to socket\n");

			int readBytes = 0;
			while((readBytes = read(fd, recvReq, reqSize)) == 0);
			recvReq[readBytes] = '\0';
				//printf("Error: read from socket\n");
			printf("got: %s\nend\n", recvReq);

			fclose(inFile);
		}
		else{
			printf("\tUnkown Command: %s\n", msg);
		}

		close(fd);
		fd = socket(AF_INET, SOCK_STREAM, 0);
		if(fd == -1)
			perror("Erro ao criar socket");

		if(connect(fd, (struct sockaddr*) &addr, sizeof(addr)) == -1){
			perror("ERROR");
			printf("erro: connect");
			return 0;
		}
		/*if (!fgets(msg, 80, stdin))
		    printf("erro: command");

		printf("%s\n", msg);

		write(fd, msg, sizeof(msg));

		read(fd, buffer, sizeof(buffer));

		printf("%s\n", buffer);*/
	}
	close(fd);

	return 0;
}
