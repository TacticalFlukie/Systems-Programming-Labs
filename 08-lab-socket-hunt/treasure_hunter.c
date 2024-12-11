// Replace PUT_USERID_HERE with your actual BYU CS user id, which you can find
// by running `id -u` on a CS lab machine.
#define USERID 1823701865
// #define USERID 123456789
#define BUFFSIZE 16
#define BUF_SIZE 500

#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <math.h>

#include "sockhelper.h"

int verbose = 0;
int debug = 0;
int port = 0;
char *server;
struct addrinfo hints;

void print_bytes(unsigned char *bytes, int byteslen);

int main(int argc, char *argv[]) {
	//Part one: Draft inital message
	
	// printf("\n");
	int level, seed = -1;
	if(argc < 5) {
		printf("ERROR: Not enough arguments!\n");
		printf("Syntax: ./treasure_hunter server port level seed\n\n");
		exit(EXIT_FAILURE);
	}
	server = argv[1];
	port = atoi(argv[2]);
	level = atoi(argv[3]);
	seed = atoi(argv[4]);

	if(debug) 
	{
	printf("Server: %s \n", server);
	printf("port: %d \n", port);
	printf("level: %d \n", level);
	printf("seed: %d \n", seed);
	fflush(stdout);
	}

	unsigned char message[8];
	/*
	* Byte 0 is 0 always,
	* Byte 1 is level as an int
	* Bytes 2-5: USERID in hex, put into network order (big endian)
	* Bytes 6-7: Seed used for RNG in network order (big endian)
	*/
	message[0] = 0x0;
	message[1] = level;
	unsigned int val = htonl(USERID);
	memcpy(&message[2], &val, sizeof(unsigned int));
	unsigned short val2 = htons(seed);
	memcpy(&message[6], &val2, sizeof(unsigned short));	

	// print_bytes(message, 8);
	

	/*
	* Set up UDP Socket (client side)
	*/
	
	// Initialize everything to 0
	memset(&hints, 0, sizeof(struct addrinfo));
	// Set the address family to AF_INET (IPv4-only)
	hints.ai_family = AF_INET;
	// Use type SOCK_DGRAM (UDP)
	hints.ai_socktype = SOCK_DGRAM;

	struct addrinfo *addrResultList;
	int s;
	char prt[snprintf(0,0,"%+d",port)-1];
	sprintf(prt, "%d", port);
	s = getaddrinfo(server, prt, &hints, &addrResultList);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}
	
	struct sockaddr_storage local_addr_ss;
	struct sockaddr *local_addr = (struct sockaddr *)&local_addr_ss;
	char local_ip[INET6_ADDRSTRLEN];
	unsigned short local_port;

	struct sockaddr_storage remote_addr_ss;
	struct sockaddr *remote_addr = (struct sockaddr *)&remote_addr_ss;
	char remote_ip[INET6_ADDRSTRLEN];
	unsigned short remote_port; 

	// memcpy(remote_ip, s, sizeof(s));
	// printf("Server: %s \nRemoteip: %s \n", server, remote_ip);
	// fflush(NULL);

	int sfd;
	socklen_t addr_len;

	struct addrinfo *currResult;
	for (currResult = addrResultList; currResult != NULL; currResult = currResult->ai_next) {
		sfd = socket(currResult->ai_family, currResult->ai_socktype, currResult->ai_protocol);
		if (sfd < 0) {
			// error creating the socket, try next
			continue;
		}

		// addr_fam = currResult->ai_family;
		addr_len = currResult->ai_addrlen;

		memcpy(remote_addr, currResult->ai_addr, sizeof(struct sockaddr_storage));

		// populate_sockaddr(remote_addr, addr_fam, NULL, remote_port);

		// if (connect(sfd, remote_addr, addr_len) >= 0)
		// 	break;
		break;

	}
	// connect() did not succeed with any address returned by
	// getaddrinfo().
	if (currResult == NULL) {
		fprintf(stderr, "Could not connect\n");
		exit(EXIT_FAILURE);
	}

	addr_len = sizeof(struct sockaddr_storage);
	s = getsockname(sfd, local_addr, &addr_len);
	parse_sockaddr(local_addr, local_ip, &local_port);


	parse_sockaddr(remote_addr, remote_ip, &remote_port);
	/*
	*END OF PART 2: SETTING UP THE SOCKET
	*PART 3: CONVERSATION WITH SOCKET
	*/

	//inital args
	unsigned char buf[BUF_SIZE];
	size_t len = 8;
	// unsigned int nonce = 0;
	do {
		ssize_t nwritten = sendto(sfd, message, len, 0, remote_addr, addr_len);

		if (nwritten < 0) {
			perror("send");
			exit(EXIT_FAILURE);
		}
		// printf("Sent %zd bytes: %s\n", len, message);

		ssize_t nread = recvfrom(sfd, buf, BUF_SIZE, 0, remote_addr, &addr_len);

		buf[nread] = '\0';
		if (nread < 0) {
			perror("read");
			exit(EXIT_FAILURE);
		}

		// printf("Received %zd bytes: %s\nPRINTBYTES:", nread, buf);

		// print_bytes(buf,nread);

		/*
		* PART 4: HANDLE RESPONSE
		*/
		unsigned char *bytes;
		bytes = buf;
		unsigned int chunkLen = bytes[0];
		if(debug)
			printf("length:%d\n",chunkLen);
		if(chunkLen == 0){
			printf("\n");
			fflush(NULL);
			exit(EXIT_SUCCESS);
		} else if(chunkLen > 127){
			//ERROR
			printf("Error Code: %c\n", bytes[0]);
			return -1;
		} //else chunkLen > 0 && < 128; read message, number is length
		unsigned char chunk[chunkLen + 1];
		memcpy(chunk, (bytes + 1), chunkLen);
		chunk[chunkLen] = '\0';
		int opcode = bytes[chunkLen + 1];
		if(debug)
			printf("opCode: %d\n", opcode);
		// if(opcode != 0) 

		unsigned char temp[2];
		memcpy(temp, (bytes + chunkLen + 2), 2);
		unsigned short opparam;
		memcpy(&opparam, temp, 2);
		opparam = ntohs(opparam);
		unsigned int nonce = 0;

		
		if(opcode == 1) {
			struct sockaddr_in *addr_in = (struct sockaddr_in *)remote_addr;
			addr_in->sin_port = htons(opparam);
			// populate_sockaddr(remote_addr, addr_fam, remote_ip, opparam);
		} else if(opcode == 2) {
			struct sockaddr_in *addr_in = (struct sockaddr_in *)local_addr;
			addr_in->sin_port = htons(opparam);
			int newsfd = socket(AF_INET, SOCK_DGRAM,0);

			if (bind(newsfd, local_addr, sizeof(struct sockaddr_storage)) < 0) {
				perror("bind()");
			}
			close(sfd);
			sfd = newsfd;
		} else if(opcode == 3){ 
			unsigned short m = opparam;
			// printf("This one is: %d\n", m);

			struct sockaddr_in server_addr;
			struct sockaddr_storage remote_addr_ss;
			struct sockaddr *remote_addr_temp = (struct sockaddr *)&remote_addr_ss;

			server_addr.sin_family = AF_INET;
			server_addr.sin_addr.s_addr = INADDR_ANY;
			server_addr.sin_port = htons(local_port);

			int newsfd = socket(AF_INET, SOCK_DGRAM,0);

			bind(newsfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
			nonce = 0;
			// print_bytes(bytes,32);
			// printf("m:%d\n", m);
			for(int i = 0; i < m; i++) {
				recvfrom(sfd, buf, sizeof(buf), 0, remote_addr_temp, &addr_len);
				nonce = nonce + ntohs(((struct sockaddr_in *)remote_addr_temp)->sin_port);
				// printf("Received datagrm from port %d\n", ntohs(((struct sockaddr_in *)remote_addr_temp)->sin_port));
				fflush(NULL);
			}
			// printf("nonce: %d\n", nonce);

			close(newsfd);
		}
		if(debug)
			printf("opParam: %d, %x\n", opparam, opparam);
		
		unsigned char temp2[4];
		memcpy(temp2, (bytes + chunkLen + 4), 4);
		if(opcode != 3){
			memcpy(&nonce, temp2, 4);
			nonce = ntohl(nonce);
		}
		if(debug)
		{
			printf("Nonce: %d, %x\n", nonce, nonce);
			printf("\n");
			printf("%x\n", chunkLen);
			printf("%s\n", chunk); // <-- Remember, this will only work
								// if you have null-terminated the chunk!
			printf("%x\n", opcode);
			printf("%x\n", opparam);
			printf("%x\n\n", nonce);
		}
		printf("%s", chunk);

		nonce++;
		// printf("nonce: %d\n", nonce);
		nonce = htonl(nonce);
		len = 4;
		memcpy(&message, &nonce, 4);
	} while(1);
	

	exit(EXIT_SUCCESS);

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
