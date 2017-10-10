#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

int fd_udp,fd_tcp, newfd, last_i;
struct hostent *hostptr;
int addrlen, ws_port = 59000, PORT = 58022;
char buffer[80], buffer_test[80];
char req[4] = "";
char fileName[40] = "";
char data[40] =""; //PLACEHOLDER


struct sockaddr_in serveraddr, serveraddr_tcp, clientaddr;
struct in_addr *a;
struct hostent *h;

int doWordCount(char* data, int charsRead){
	/*FILE *fp1;
	char a;
	int count = 0;
	fp1 = fopen(fileName, "r");
	while (a != EOF){
		a = fgetc(fp1);
		if (a == ' '){
			count++;
		}
	}
	count++;
	fclose(fp1);
	printf("%d\n", count);
	return count;*/
	int i, count = 0;
	for (i = 0; i < charsRead; i++){
		if (data[i] == ' '){
			count++;
		}
	}
	count++;
	return count;
}

char* findLongestWord(char* fileName, char* data){ //prob wrong af
	FILE *fp1;
	char* a = (char*) malloc(sizeof(data));
	fp1 = fopen(fileName, "r");
	char* longestWord;
	while (fgets(a, sizeof(data), fp1)){
		a[strlen(a)-1] = 0;
		char* word;
		int maxLenght = 0;
		longestWord = (char*) calloc(40, sizeof(char));
		word = strtok(a, " ");
		while(word != NULL){
			if (strlen(word) > maxLenght){
				maxLenght = strlen(word);
				strcpy(longestWord, word);
			}
			word = strtok(a, " ");
		}
	}
	fclose(fp1);
	printf("%s\n", longestWord);
	return longestWord;
}

char* convertUpper(char* data, int charsRead){
	int i;
	for (i = 0; i < charsRead; i++){
		data[i] = toupper(data[i]);
	}
	return data;
}

char* convertLower(char* data, int charsRead){
	/*FILE *fp1;
	char a;
	fp1 = fopen(fileName, "r");
	while (a != EOF){
		a = fgetc(fp1);
		a = tolower(a);
		printf("%c", a);
	}
	printf("\n");
	fclose(fp1);*/
	int i;
	for (i = 0; i < charsRead; i++){
		data[i] = tolower(data[i]);
	}
	return data;
}

int main(int argc, char** argv){
	fd_udp = socket(AF_INET, SOCK_DGRAM, 0);
	fd_tcp = socket(AF_INET, SOCK_STREAM, 0);
	if(fd_udp == -1)
		perror("Erro ao criar socket");
	if(fd_tcp == -1)
		perror("Erro ao criar socket TCP");
	char msg[80] = "";
	char hostName[128], name[128], ip_ws[20];
	char ws_port_string[1];
	int argv_size = (argc - 1) * 3 + (argc - 1); //alocacao de espaco mal, antes so considerava os PTCs, ta a alocar mais do que precisa
	char* reg_msg = (char*)malloc(21 + argv_size);
	char* unreg_msg = (char*)malloc(25);
	char size[16];
	int size_int;
	strcat(reg_msg, "REG ");
	int j, i;
	for (j = 1; j < argc ; j++){
		if (!strcmp(argv[j], "WCT") || (!strcmp(argv[j], "LOW")) || (!strcmp(argv[j], "UPP")) || (!strcmp(argv[j], "FLW"))){
			strcat(reg_msg, argv[j]);
			strcat(reg_msg, " ");
		}
		else if (!strcmp(argv[j], "-p")){
				ws_port = atoi(argv[j+1]);
			}
		else if (!strcmp(argv[j], "-n")){
				strcpy(hostName, argv[j+1]);
			}
		else if (!strcmp(argv[j], "-e")){
				PORT = atoi(argv[j+1]);
			}
	}

	gethostname(name, 128);
	h = gethostbyname(name);
	a=(struct in_addr*)h->h_addr_list[0];


	strcat(reg_msg, inet_ntoa(*a));
	strcat(reg_msg, " ");
	sprintf(ws_port_string, "%d", ws_port);
	strcat(reg_msg, ws_port_string);
	strcat(reg_msg, "\n");
	printf("%s", reg_msg);

	strcat(unreg_msg, "UNR ");
	strcat(unreg_msg, inet_ntoa(*a));
	strcat(unreg_msg, " ");
	strcat(unreg_msg, ws_port_string);
	strcat(unreg_msg, "\n");
	printf("%s", unreg_msg);

	if(gethostname(hostName, 128)==-1)
		printf("erro: gethostname\n");

	hostptr = gethostbyname(hostName);


	memset((void*) &serveraddr, (int)'\0', sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = ((struct in_addr*) (hostptr->h_addr_list[0]))->s_addr;
	serveraddr.sin_port = htons((u_short)PORT);

	addrlen = sizeof(serveraddr);

	if(sendto(fd_udp, reg_msg, strlen(reg_msg), 0, (struct sockaddr*) &serveraddr, sizeof(serveraddr)) == -1){
		perror("Error sending register message");
		return 1;
	}

	memset((void*) &serveraddr_tcp, (int)'\0', sizeof(serveraddr_tcp));
	serveraddr_tcp.sin_family = AF_INET;
	serveraddr_tcp.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr_tcp.sin_port = htons((u_short)ws_port);


	if(bind(fd_tcp, (struct sockaddr*) &serveraddr_tcp, sizeof(serveraddr_tcp)) == -1)
		perror("Error binding socket Tcp");

	listen(fd_tcp, 5);

	while(1){
		addrlen = sizeof(clientaddr);
		newfd = accept(fd_tcp, (struct sockaddr*) &clientaddr, &addrlen);
		if (newfd == -1){
			perror("Error accept");
		}
		while(read(newfd, buffer, sizeof(buffer)-1) == 0);
		buffer[79] = '\0';
		int j = 0, charsRead = 0;
		for(i = 21; buffer[i] != ' '; i++){
					size[j] = buffer [i];
					j++;
		}
		size[j] = '\0';
		size_int = atoi(size);
		printf("%d\n", size_int);
		char *fileInBuffer = malloc(sizeof(char)*size_int+1);
		fileInBuffer[0] = '\0';
		while(charsRead<size_int){
			int tempChars = read(newfd, buffer_test, sizeof(buffer_test)-1);
			buffer_test[tempChars] = '\0';
			strcat(fileInBuffer, buffer_test);
			printf("FIB: %s | BUFFER %s | tempChars: %d\n", fileInBuffer, buffer_test, tempChars);

			if(tempChars == -1)
				perror("ERROR: reading rest of file");
			else{
				charsRead += tempChars;
			}
		}

		//printf("%s\n", fileInBuffer);

		if(!strncmp(buffer, "WRQ ", 4)){
			for(i = 4; i < 7; i++)
				req[i-4] = buffer[i];
			for (i = 8; i < 20; i++)
				fileName[i - 8] = buffer[i];
			req[3] = '\0';
			fileName[12] = '\0';
			printf("REQ: %s | fileName: %s\n", req, fileName);
			if(!strcmp(req, "WCT")){
				int wrd_count = 0;
				char* rep_msg = (char*) malloc(sizeof(buffer));
				rep_msg[0] = '\0';
				wrd_count = doWordCount(data, charsRead);
				strcat(rep_msg, "REP R ");
				//strcat(rep_msg, strlen(wrd_count));
				strcat(rep_msg, " ");
				strcat(rep_msg, data);
				printf("%s\n", rep_msg);
			}
			else if(!strcmp(req, "FLW")){
				/*char* longest_word;
				char* rep_msg = (char*) malloc(sizeof(buffer));
				longest_word = findLongestWord(fileName, data);
				strcat(rep_msg, "REP R ");
				strcat(rep_msg, size);
				strcat(rep_msg, data);*/
				//process reply message with result
			}
			else if(!strcmp(req, "UPP")){
				printf("In Convert Upper\n");
				char* rep_msg = malloc(sizeof(char) * (size_int + strlen(size) + 8));
				rep_msg[0] = '\0';
				char* data = convertUpper(fileInBuffer, charsRead);
				strcat(rep_msg, "REP F ");
				strcat(rep_msg, size);
				strcat(rep_msg, " ");
				strcat(rep_msg, data);
				printf("%s\n", rep_msg);
				if(write(newfd, rep_msg, strlen(rep_msg)) == -1)
					perror("ERROR: write to working server");
			}
			else if(!strcmp(req, "LOW")){
				printf("In Convert Lower\n");
				char* rep_msg = malloc(sizeof(char) * (size_int + strlen(size) + 8));
				rep_msg[0] = '\0';
				char* data = convertLower(fileInBuffer, charsRead);
				strcat(rep_msg, "REP F ");
				strcat(rep_msg, size);
				strcat(rep_msg, " ");
				strcat(rep_msg, data);
				printf("%s\n", rep_msg);
				if(write(newfd, rep_msg, strlen(rep_msg)) == -1)
					perror("ERROR: write to working server");
			}
			else{
				//write ("WRP EOF");
			}
		}
		else{
			// write ("WRP ERR");
		}
	}
	close(fd_tcp);
	close(newfd);
	close(fd_udp);
	return 0;
}
