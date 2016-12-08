#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <time.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sched.h>
#include <signal.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024
#define WAITING 8
#define SERVER "Server: Apache/2.4.7 (Ubuntu)"

//#include <winsock.h>
int time_tick;
const char *PATH = "/home/namhyun13";
struct sockaddr_in addr;


void setup_init(int *ser_socket);
void respond(int client);
void send_header(int client, const char *path, char *content_header);
void send_content(int client, const char *path);
void set_port(u_short *setport);



void set_port(u_short *setport) {
	u_short input;
	printf("type the port you want to assign: ");
	scanf("%hu", &input);
	while (input < 1023 || input >9999) {
		printf("type again: 1024~9999\n");
		scanf("%hu", &input);
	}
	printf("\nPort set to : %hu\n\n", input);
	*setport = input;
}

void setup_init(int *ser_socket) {
	u_short port;
	int addr_len = sizeof(addr);

	set_port(&port);

	*ser_socket = socket(AF_INET, SOCK_STREAM, 0);		//PF?? or AF???
	if (*ser_socket == -1) {
		perror("socket");
		exit(1);
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);	//host byte to network byte order
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(*ser_socket, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
		;
	}
	else {
		perror("bind");
		exit(1);
	}
	if (listen(*ser_socket, 0) == -1) {		// backlog depends
		perror("listen");
		exit(1);
	}
}


void send_header(int client, const char *path, char content_header[24]) {
	char buffer[BUF_SIZE];
	char file_dest[90];
	char weekday[3]; char c_month[3];
	char *temp;
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	struct stat attr;
	int size;
	
	strcpy(file_dest, path);
	if (stat(file_dest, &attr) != 0) {
		strcpy(buffer, "WRONG DIRECTION::COULD NOT FIND THE PATH\n");
		write(client, buffer, sizeof(buffer));
		exit(1);
	}

	switch (tm.tm_wday) {
	case 0: strcpy(weekday, "Sun"); break;	case 1: strcpy(weekday, "Mon"); break;	case 2: strcpy(weekday, "Tue"); break;
	case 3: strcpy(weekday, "Wed"); break;	case 4: strcpy(weekday, "Thu"); break;	case 5: strcpy(weekday, "Fri"); break;
	case 6: strcpy(weekday, "Sat"); break;
	}
	switch (tm.tm_mon) {
	case 0: strcpy(c_month, "Jan"); break;	case 1: strcpy(c_month, "Feb"); break;	case 2: strcpy(c_month, "Mar"); break;
	case 3: strcpy(c_month, "Apr"); break;	case 4: strcpy(c_month, "May"); break;	case 5: strcpy(c_month, "Jun"); break;
	case 6: strcpy(c_month, "Jul"); break;	case 7: strcpy(c_month, "Aug"); break;	case 8: strcpy(c_month, "Sep"); break;
	case 9: strcpy(c_month, "Oct"); break;	case 10: strcpy(c_month, "Nov"); break;	case 11: strcpy(c_month, "Dec"); break;
	}

	memset(buffer, 0, sizeof(buffer));
	strcpy(buffer, "HTTP/1.1 200 OK\n");
	write(client, buffer, sizeof(buffer));
	sprintf(buffer, "Date: %s, %02d %s %d %02d:%02d:%02d GMT\n", weekday, tm.tm_mday, c_month, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
	write(client, buffer, sizeof(buffer));
	strcpy(buffer, SERVER);
	write(client, buffer, sizeof(buffer));
	sprintf(buffer, "Last modified time: %s", ctime(&attr.st_mtime));
	write(client, buffer, sizeof(buffer));
	//Etag or Ctag  = no idea
	memset(buffer, 0, sizeof(buffer));
	strcpy(buffer, "Accept-Ranges: none\n");
	write(client, buffer, sizeof(buffer));
	size = attr.st_size;
	sprintf(buffer, "Content-Length: %d\n" ,size);
	write(client, buffer, sizeof(buffer));
	//Vary-Encoding--- give up
	strcpy(buffer, "Connection: close\n"); //whatever. prob closed when im doing this
	write(client, buffer, sizeof(buffer));
	sprintf(buffer, "Content-Type: %s\n\n", content_header);
	write(client, buffer, sizeof(buffer));
}

void send_content(int client, const char *path) {
	FILE *indexfile = NULL;
	char file_dst[1024];
	char buffer[1024];
	int i; int read_byte;
	strcpy(file_dst, path);
		
	memset(buffer, 0, sizeof(buffer));
	indexfile = fopen(file_dst, "r");
	if (indexfile == NULL) {
		strcpy(buffer, "COULD NOT OPEN THE FILE\n");
		write(client, buffer, sizeof(buffer));
		fclose(indexfile);
		exit(0);
	}
	while (!feof(indexfile)) {
		read_byte = fread(buffer, sizeof(char), sizeof(buffer), indexfile);
		read_byte = write(client, buffer, read_byte);

	}


	while (fgets(buffer, 1024, indexfile) != NULL) {
		if (!(write(client, buffer, sizeof(buffer)))) {
			printf("FILE SENDING ERROR\n");
			strcpy(buffer, "COULD NOT SEND THE FILE\n");
			write(client, buffer, sizeof(buffer));
		}
	}

	fclose(indexfile);
}

void respond(int client) {
	char buffer[BUF_SIZE];
	char command[6];
	char *token;
	char path[124];
	char filename[100];
	int option_index = 0;
	char content_type[24];
	int i;
	int len;
	int need = 1;


	token = (char *)malloc(sizeof(char));

	//printf("Client Socket : %d\n", client);
	strcpy(path, PATH);
	memset(buffer, 0, sizeof(buffer));

	if ((read(client, buffer, sizeof(buffer))) < 0) {
		perror("Request not valid\n");
		close(client);
		exit(1);
	}

	printf("Request : %s \n", buffer);
	i = 0;

	token = strtok(buffer, " ");
	strcpy(command, token);
	token = strtok(NULL, " ");
	strcpy(filename, token);

	for (i = 2; i < 4; i++) token = strtok(NULL, " ");
	strcpy(content_type, token);
	while (token = strtok(NULL, " "));
	strcat(path, filename);
	if (strcmp(command, "GET") == 0) {
		if (strcmp(filename, "/favicon.ico") == 0) {
			memset(buffer, 0, sizeof(buffer));
			printf("@@@@@@@@@@@@@@@@@@@@@@");
			sprintf(buffer, "HTTP/1.0 204 NO CONTENT\r\n\r\n");
			send(client, buffer, strlen(buffer), 0);
			close(client);
		}		
		else {
			send_header(client, path, content_type);
			send_content(client, path);
		}
	}
	else {
		memset(buffer, 0, sizeof(buffer));
		strcpy(buffer, "Unsupported command\n");
		write(client, buffer, sizeof(buffer));
	}
}


int main(int argc, char* argv[]) {
	int ser_socket; int cli_socket;
	pid_t pid;
	int number = 0; 
	int addr_len;

	setup_init(&ser_socket);
	//server started

	//select(STDIN+1, &readfds, &writefds, &exceptfds, timeout)
	//http://wiki.kldp.org/Translations/html/Socket_Programming-KLDP/Socket_Programming-KLDP.html#select


	while (1) {
		/*accept: int accept(int socket, struct sockaddr *restrict address, socklen_t *restrict address_len); 
		address= pointer to sockaddr where the address of the connecting socket will be returned
		address_len = sizeof(struct sockaddr_in 'address');
		
		must receive value because the value describes accepted socket
		*/
		addr_len = sizeof(addr);
		if ((cli_socket = accept(ser_socket, (struct sockaddr *)&addr, (socklen_t*)&addr_len)) < 0) {
			perror("accept");
			exit(0);
		}
	
		if ((pid = fork()) < 0) {
			perror("fork");
			close(ser_socket);	
			close(cli_socket);
			exit(0);
		}
		else if (pid == 0) {
			close(ser_socket);
			respond(cli_socket);
			close(cli_socket);
			exit(0);
		}
		else {
			close(cli_socket);
		}	
	}
	close(ser_socket);
	ser_socket = -1;
	printf("Server closed\n");
	return 0;
}

