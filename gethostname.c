#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

int main(){
	char buffer[128];
	gethostname(buffer, 128);
	printf("%s\n", buffer);
	return 0;
}
