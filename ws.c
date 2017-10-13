
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
#include <sys/wait.h>
#include <math.h>

int fd_udp,fd_tcp,cs_tcp, newfd, last_i;
char size_string[16], size_WCT[16], digit_WCT[16];
struct hostent *hostptr;
int addrlen, ws_port = 59000, PORT = 58022;
char buffer[80], buffer_test[80];
char req[4] = "";
char fileName[40] = "";
char data[40] = "", name[128]; //PLACEHOLDER

struct sockaddr_in serveraddr, serveraddr_tcp, clientaddr;
struct in_addr *a;
struct hostent *h;

void termHandler(int sig)
{
  //send unreg msg to ws them return 0;
	char unregMsg[80] = "";
	int i;
	alarm(2);

	/*strcat(unreg_msg, "UNR ");
	strcat(unreg_msg, inet_ntoa(*a));
	strcat(unreg_msg, " ");
	strcat(unreg_msg, ws_port_string);
	strcat(unreg_msg, "\n");
	printf("%s", unreg_msg);*/

	sprintf(unregMsg, "UNR %s %d\n", inet_ntoa(*a), ws_port); //UNR IPWS portWS
	if(sendto(fd_udp, unregMsg, strlen(unregMsg), 0, (struct sockaddr*) &serveraddr, addrlen) == -1){
		perror("Error sending register message");
		close(fd_udp);
		close(fd_tcp);
		close(cs_tcp);
		exit(1);
	}

	int recvBytes = recvfrom(fd_udp, unregMsg, sizeof(unregMsg), 0, (struct sockaddr*) &serveraddr, (socklen_t *)&addrlen);  //RAK OK/NOK
	unregMsg[recvBytes] = '\0';
	if(recvBytes != 7){
		perror("ERROR: unexpected cs msg\n");
		close(fd_udp);
		close(fd_tcp);
		close(cs_tcp);
		exit(1);
	}
	for(i = 0; i<recvBytes && i<80; i++){
		if(unregMsg[i] != "UAK OK\n"[i])
			break;
	}
	if(i != recvBytes){
		perror("ERROR: unexpected cs msg\n");
	}
	close(fd_udp);
	close(fd_tcp);
	close(cs_tcp);
	exit(1);

}

void catch_alarm (int sig)
{
  kill(getpid(), SIGKILL);
}

int doWordCount(char* data, int charsRead){
	int count = 0, temp_count, control = 0;
	int nDigits = 0, i;
	for (i = 0; i < charsRead; i++){
		while(i < charsRead && data[i] != '\0' && data[i] != '\n' && data[i] != ' ' && ((97 <= (int)data[i] && (int)data[i] <= 122) || (65 <= (int)data[i] && (int)data[i] <= 90))){
				control = 1;
				i++;
		}
		if (control){
			count++;
			control = 0;
		}

	}
	temp_count = count;
	while (temp_count >= 1){
		temp_count = temp_count / 10;
		nDigits++;
	}
	//nDigits = floor(log10(abs(count))) + 1;
	sprintf(digit_WCT, "%d", nDigits);
	return count;
}

char* findLongestWord(char* data, int charsRead){
	//printf("OLa\n");
	int size, i, j=0, first_entry = 0;
	char* word = malloc(sizeof(char*) * charsRead);
	word[0] = '\0';
	char* longestWord = malloc(sizeof(char*) * charsRead);
	longestWord[0] = '\0';
	for (i = 0; i < charsRead; i++){
		while(i < charsRead && data[i] != '\0' && data[i] != '\n' && data[i] != ' ' && ((97 <= (int)data[i] && (int)data[i] <= 122) || (65 <= (int)data[i] && (int)data[i] <= 90))){
				word[j++] = data[i++];
		}
		if (j != 0){
			word[j] = '\0';
			if (!first_entry){
				first_entry = 1;
				strcpy(longestWord, word);
				size = strlen(longestWord);
			}
			if (strlen(word) > strlen(longestWord)){
				strcpy(longestWord, word);
				size = strlen(longestWord);
			}
			j = 0;
		}

	}
	sprintf(size_string, "%d", size);
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
	int i;
	for (i = 0; i < charsRead; i++){
		data[i] = tolower(data[i]);
	}
	return data;
}

int main(int argc, char** argv){
	fd_udp = socket(AF_INET, SOCK_DGRAM, 0);
	fd_tcp = socket(AF_INET, SOCK_STREAM, 0);
	cs_tcp = socket(AF_INET, SOCK_STREAM, 0);
	if(fd_udp == -1)
		perror("Erro ao criar socket");
	if(fd_tcp == -1)
		perror("Erro ao criar socket TCP");

	signal(SIGTERM, termHandler);
	signal(SIGINT, termHandler);
	signal (SIGALRM, catch_alarm);

	char hostName[128];
	char ws_port_string[1];
	int argv_size = (argc - 1) * 3 + (argc - 1); //alocacao de espaco mal, antes so considerava os PTCs, ta a alocar mais do que precisa
	char* reg_msg = (char*)malloc(21 + argv_size);
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
	//printf("%s", reg_msg);

	if(gethostname(hostName, 128)==-1)
		perror("erro: gethostname\n");

	hostptr = gethostbyname(hostName);


	memset((void*) &serveraddr, (int)'\0', sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = ((struct in_addr*) (hostptr->h_addr_list[0]))->s_addr;
	serveraddr.sin_port = htons((u_short)PORT);

	addrlen = sizeof(serveraddr);

	if(sendto(fd_udp, reg_msg, strlen(reg_msg), 0, (struct sockaddr*) &serveraddr, addrlen) == -1){
		perror("Error sending register message");
		close(fd_udp);
		close(fd_tcp);
		close(cs_tcp);
		return 1;
	}

	int recvBytes = recvfrom(fd_udp, reg_msg, sizeof(reg_msg), 0, (struct sockaddr*) &serveraddr, (socklen_t *)&addrlen);  //RAK OK/NOK
	//printf("%s\n", reg_msg);
	if(recvBytes != 7){
		perror("ERROR: unexpected cs msg\n");
		close(fd_udp);
		close(fd_tcp);
		close(cs_tcp);
		return 1;
	}
	for(i = 0; i<recvBytes; i++){
		if(reg_msg[i] != "RAK OK\n"[i])
			break;
	}
	if(i != recvBytes){
		perror("ERROR: unexpected cs msg\n");
		close(fd_udp);
		close(fd_tcp);
		close(cs_tcp);
		return 1;
	}

	memset((void*) &serveraddr_tcp, (int)'\0', sizeof(serveraddr_tcp));
	serveraddr_tcp.sin_family = AF_INET;
	serveraddr_tcp.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr_tcp.sin_port = htons((u_short)ws_port);


	if(bind(fd_tcp, (struct sockaddr*) &serveraddr_tcp, sizeof(serveraddr_tcp)) == -1)
		perror("Error binding socket Tcp");

	listen(fd_tcp, 5);

	memset((void*) &serveraddr, (int)'\0', sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = ((struct in_addr*) (hostptr->h_addr_list[0]))->s_addr;
	serveraddr.sin_port = htons((u_short)PORT);
	addrlen = sizeof(serveraddr);

	while(1){
		addrlen = sizeof(clientaddr);
		newfd = accept(fd_tcp, (struct sockaddr*) &clientaddr, (socklen_t *)&addrlen);
		if (newfd == -1){
			perror("Error accept");
		}

		int childPid = fork();

		if(childPid == 0){ //child code
			while(read(newfd, buffer, sizeof(buffer)-1) == 0);
			buffer[79] = '\0';
			//printf("BUFF: %s\n", buffer);
			int j = 0, charsRead = 0;
			for(i = 21; buffer[i] != ' '; i++){
						size[j] = buffer [i];
						j++;
			}
			size[j] = '\0';
			size_int = atoi(size);
			//printf("size data: %d\n buffer size: %d\n", size_int, (int)strlen(buffer));
			char *fileInBuffer = malloc(sizeof(char)*size_int+1);
			fileInBuffer[0] = '\0';
			for(j = ++i; i<strlen(buffer); i++){
				fileInBuffer[i-j] = buffer[i];
				charsRead++;
			}

			//printf("size data: %d\n charsread: %d\n", size_int, charsRead);

			while(charsRead<size_int-1){
				int tempChars = read(newfd, buffer_test, sizeof(buffer_test)-1);
				buffer_test[tempChars] = '\0';
				//printf("FIB: %s | BUFFER %s | tempChars: %d\n", fileInBuffer, buffer_test, tempChars);
				strcat(fileInBuffer, buffer_test);

				if(tempChars == -1)
					perror("ERROR: reading rest of file");
				else{
					charsRead += tempChars;
				}
				//printf("Read Already: %d; Read Now: %d;\n", charsRead, tempChars);
			}

			//printf("%s\n", fileInBuffer);

			if(!strncmp(buffer, "WRQ ", 4)){
				for(i = 4; i < 7; i++)
					req[i-4] = buffer[i];
				for (i = 8; i < 20; i++)
					fileName[i - 8] = buffer[i];
				req[3] = '\0';
				fileName[12] = '\0';
				char* rep_msg;
				//printf("REQ: %s | fileName: %s\n", req, fileName);
				if(!strcmp(req, "WCT")){
					int wrd_count = 0;
					rep_msg = malloc(sizeof(char*) * size_int + strlen(size) + 8);
					rep_msg[0] = '\0';
					wrd_count = doWordCount(fileInBuffer, charsRead);
					sprintf(size_WCT, "%d", wrd_count);
					strcat(rep_msg, "REP R ");
					strcat(rep_msg, digit_WCT);
					strcat(rep_msg, " ");
					strcat(rep_msg, size_WCT);
					//printf("%s\n", rep_msg);
				}
				else if(!strcmp(req, "FLW")){
					char* longest_word;
					rep_msg = malloc(sizeof(char*) * size_int + strlen(size) + 8);
					rep_msg[0] = '\0';
					longest_word = findLongestWord(fileInBuffer, charsRead);
					strcat(rep_msg, "REP R ");
					strcat(rep_msg, size_string);
					strcat(rep_msg, " ");
					strcat(rep_msg, longest_word);
					//process reply message with result
				}
				else if(!strcmp(req, "UPP")){
					//printf("In Convert Upper\n");
					rep_msg = malloc(sizeof(char) * (size_int + strlen(size) + 8));
					rep_msg[0] = '\0';
					char* data = convertUpper(fileInBuffer, charsRead);
					strcat(rep_msg, "REP F ");
					strcat(rep_msg, size);
					strcat(rep_msg, " ");
					strcat(rep_msg, data);
				}
				else if(!strcmp(req, "LOW")){
					rep_msg = malloc(sizeof(char) * (size_int + strlen(size) + 8));
					rep_msg[0] = '\0';
					char* data = convertLower(fileInBuffer, charsRead);
					strcat(rep_msg, "REP F ");
					strcat(rep_msg, size);
					strcat(rep_msg, " ");
					strcat(rep_msg, data);
				}
				else{
					//write ("WRP EOF");
				}
				int t = write(newfd, rep_msg, strlen(rep_msg));
				if(t == -1)
					perror("ERROR: write to working server");

				close(newfd);

			}
			else if(childPid == -1){
				perror("ERROR: create child");
			}
			else{ // parent code
				close(newfd);
			}
			close(newfd);
			return 0;
		}
		else{
			//write (newfd, "WRP ERR");
		}
	}
	close(fd_tcp);
	close(newfd);
	close(fd_udp);
	return 0;
}
