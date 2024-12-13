#include <stdio.h>

#include "sockhelper.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

/* Recommended max object size */
#define MAX_OBJECT_SIZE 102400
#define BUF_SIZE 1024

static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:97.0) Gecko/20100101 Firefox/97.0";

int complete_request_received(char *);
void parse_request(char *, char *, char *, char *, char *);
void test_parser();
void print_bytes(unsigned char *, int);
int open_sfd(int port);
void handle_client(int fd, char *stre);


struct addrinfo hints;

int main(int argc, char *argv[])
{
	// test_parser();
	int sfd = open_sfd(atoi(argv[1]));
	
	listen(sfd,100);
	int acceptfd = -420;
	while(1) {
		struct sockaddr_storage remote_addr_ss;
		socklen_t addr_len = sizeof(struct sockaddr_storage);
		struct sockaddr *remote_addr = (struct sockaddr *)&remote_addr_ss;
		acceptfd = accept(sfd, remote_addr, &addr_len);
		char str[BUF_SIZE];
		handle_client(acceptfd, str);
		printf("%s\n", str);
		printf("---------------------------------------------\n");
	}
	
	printf("%s\n", user_agent_hdr);
	return 0;
}

/*
*Returns fd of new socket on success, exit on failure?
*/
int open_sfd(int port)
{
	// Initialize everything to 0
	memset(&hints, 0, sizeof(struct addrinfo));
	// Set the address family to AF_INET (IPv4-only)
	int addr_fam = AF_INET;
	// Use type SOCK_DGRAM (UDP)
	int sock_type = SOCK_STREAM;

	int sfd;
	if ((sfd = socket(addr_fam, sock_type, 0)) < 0) {
		perror("Error creating socket");
		printf("UH OH\n");
		fflush(NULL);
		exit(EXIT_FAILURE);
	}

	int optval = 1;
	setsockopt(sfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

	struct sockaddr_storage local_addr_ss;
	struct sockaddr *local_addr = (struct sockaddr *)&local_addr_ss;

	// Populate local_addr with the port using populate_sockaddr().
	populate_sockaddr(local_addr, addr_fam, NULL, port);

	if (bind(sfd, local_addr, sizeof(struct sockaddr_storage)) < 0) {
		perror("Could not bind");
		exit(EXIT_FAILURE);
	}

	return sfd;
}

void handle_client(int fd, char *str)
{
	char buf[BUF_SIZE];
	// printf("sfd: %d",fd);

	while(1) {
		int nread = recv(fd, buf, 1024, 0);
		if(nread == 0) {
			close(fd);
			break;
		} else if (nread < 0) {
			perror("read");
			exit(EXIT_FAILURE);
		}
		// printf("nread: %d", nread);
		buf[nread] = '\0';
		// print_bytes((unsigned char *)buf, nread);
		char method[16], hostname[64], port[8], path[64];
		parse_request(buf, method, hostname, port, path);

		int curr = 0;

		{
			strcpy(&str[curr], method);
			curr = strlen(method);
			strcpy(&str[curr], path);
			curr += strlen(path);
			str[curr] = '\r';
			curr++;
			str[curr] = '\n';
			curr++;
			// strcpy(&str[curr], hostname);
			// curr += strlen(hostname);
			
			//LINE 2: HOST
			if(atoi(port) == 80){
				char temp[] = "Host: ";
				strcpy(&str[curr], &temp[0]);
				curr += strlen(temp);
				strcpy(&str[curr], hostname);
				curr += strlen(hostname);
			} else {
				char temp[] = "Host: ";
				strcpy(&str[curr], &temp[0]);
				curr += strlen(temp);
				strcpy(&str[curr], hostname);
				curr += strlen(hostname);
				str[curr] = ':';
				curr++;
				strcpy(&str[curr], port);
				curr += strlen(port);
			}
			str[curr] = '\r';
			curr++;
			str[curr] = '\n';
			curr++;

			//LINE 3: USERAGENT
			{
				char temp[] = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:68.0) Gecko/20100101 Firefox/68.0\r\n";
				strcpy(&str[curr], &temp[0]);
				curr += strlen(temp);
			}

			//LINE 4: CONNECTION
			{
				char temp[] = "Connection: close";
				strcpy(&str[curr], &temp[0]);
				curr += strlen(temp);
			}
			str[curr] = '\r';
			curr++;
			str[curr] = '\n';
			curr++;

			//LINE 5: PROXY-CONNECTION
			{
				char temp[] = "Proxy-Connection: close";
				strcpy(&str[curr], &temp[0]);
				curr += strlen(temp);
			}
			str[curr] = '\r';
			curr++;
			str[curr] = '\n';
			curr++;

			//LINE LAST: End Of String Char
			str[curr] = '\0';
		}

		// printf("NEWREQUEST:\n%s\n", str);

		// printf("method:%s\n", method);
		// printf("hostname:%s\n", hostname);
		// printf("port:%s\n",port);
		// printf("path:%s\n", path);

		//MORE MORE MORE ITS CHECKPOINT 3, COMMUNICATE WITH SERVER TIME BABY!

		close(fd);
		break;
	}

	return;
}

int complete_request_received(char *request) {
	char *i = strstr(request, "\r\n\r\n");
	if(i == NULL){
		return 0;
	} else {
		// printf("gottem\n");
		return 1;
	}
}

void parse_request(char *request, char *method,
		char *hostname, char *port, char *path) {
		
		//REQUEST
		char *endOfMethod = strstr(request, " ");
		endOfMethod += 1;
		strncpy(method, request, (endOfMethod - request));
		method[endOfMethod - request] = '\0';
		
		//URL
		char *endOfURL = strstr(endOfMethod, " ");
		endOfURL += 1;

		//PORT
		char *startOfPort = strstr(endOfMethod, "://");
		char *endOfLine = strstr(endOfMethod, "\r\n");
		if(startOfPort != NULL)
		{
			startOfPort++;
			if (strstr(startOfPort, ":") == NULL) {
				startOfPort = endOfLine + 1;
			} else
				startOfPort = strstr(startOfPort, ":");
		}
		if(startOfPort == NULL) 
		{
			printf("OHNO");
			exit(-1);
		}
		if(startOfPort > endOfLine) { //TYPE TWO
			char *temp = "80";
			strncpy(port, temp, 3);

			//HOSTNAME
			char *startOfHostname = strstr(endOfMethod, "://");
			startOfHostname += 3;
			char *endOfHostname = strstr(startOfHostname, "/");
			strncpy(hostname, startOfHostname, (endOfHostname - startOfHostname));
			hostname[(endOfHostname - startOfHostname)] = '\0';
			
			//PATH
			strncpy(path, endOfHostname, (endOfLine - endOfHostname));
			path[(endOfLine - endOfHostname)] = '\0';


		} else { // TYPE ONE
			//startOfPort goes up because its non inclusive, endOfPort doesn't because path includes the '/' in the value
			startOfPort++;
			char *endOfPort = strstr(startOfPort, "/");
			strncpy(port, startOfPort, (endOfPort - startOfPort));
			port[endOfPort - startOfPort] = '\0';

			//HOSTNAME
			char *startOfHostname = strstr(endOfMethod, "://");
			if(startOfHostname != NULL) 
			{
				startOfHostname += 3;
				char *endOfHostname = strstr(startOfHostname, ":");
				strncpy(hostname, startOfHostname, (endOfHostname - startOfHostname));
				hostname[(endOfHostname - startOfHostname)] = '\0';
			}
			
			//PATH
			strncpy(path, endOfPort, (endOfLine - endOfPort));
			path[(endOfLine - endOfPort) - 1] = '0';
			path[(endOfLine - endOfPort)] = '\0';
		}		
}

void test_parser() {
	int i;
	char method[16], hostname[64], port[8], path[64];

       	char *reqs[] = {
		"GET http://www.example.com/index.html HTTP/1.0\r\n"
		"Host: www.example.com\r\n"
		"User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:68.0) Gecko/20100101 Firefox/68.0\r\n"
		"Accept-Language: en-US,en;q=0.5\r\n\r\n",

		"GET http://www.example.com:8080/index.html?foo=1&bar=2 HTTP/1.0\r\n"
		"Host: www.example.com:8080\r\n"
		"User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:68.0) Gecko/20100101 Firefox/68.0\r\n"
		"Accept-Language: en-US,en;q=0.5\r\n\r\n",

		"GET http://localhost:1234/home.html HTTP/1.0\r\n"
		"Host: localhost:1234\r\n"
		"User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:68.0) Gecko/20100101 Firefox/68.0\r\n"
		"Accept-Language: en-US,en;q=0.5\r\n\r\n",

		"GET http://www.example.com:8080/index.html HTTP/1.0\r\n",

		NULL
	};
	
	for (i = 0; reqs[i] != NULL; i++) {
		printf("Testing %s\n", reqs[i]);
		if (complete_request_received(reqs[i])) {
			printf("REQUEST COMPLETE\n");
			parse_request(reqs[i], method, hostname, port, path);
			printf("METHOD: %s\n", method);
			printf("HOSTNAME: %s\n", hostname);
			printf("PORT: %s\n", port);
			printf("PATH: %s\n", path);
			printf("---------------------------------------------------------------------\n");
		} else {
			printf("REQUEST INCOMPLETE\n");
		}
	}
}

void print_bytes(unsigned char *bytes, int byteslen) {
	int i, j, byteslen_adjusted;

	if (byteslen % 8) {
		byteslen_adjusted = ((byteslen / 8) + 1) * 8;
	} else {
		byteslen_adjusted = byteslen;
	}
	for (i = 0; i < byteslen_adjusted + 1; i++) {
		if (!(i % 8)) {
			if (i > 0) {
				for (j = i - 8; j < i; j++) {
					if (j >= byteslen_adjusted) {
						printf("  ");
					} else if (j >= byteslen) {
						printf("  ");
					} else if (bytes[j] >= '!' && bytes[j] <= '~') {
						printf(" %c", bytes[j]);
					} else {
						printf(" .");
					}
				}
			}
			if (i < byteslen_adjusted) {
				printf("\n%02X: ", i);
			}
		} else if (!(i % 4)) {
			printf(" ");
		}
		if (i >= byteslen_adjusted) {
			continue;
		} else if (i >= byteslen) {
			printf("   ");
		} else {
			printf("%02X ", bytes[i]);
		}
	}
	printf("\n");
	fflush(stdout);
}
