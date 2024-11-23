// Replace PUT_USERID_HERE with your actual BYU CS user id, which you can find
// by running `id -u` on a CS lab machine.
#define USERID 1823701865
// #define USERID 123456789
#define BUFFSIZE 16

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include "sockhelper.h"

int verbose = 0;

void print_bytes(unsigned char *bytes, int byteslen);

int main(int argc, char *argv[]) {
	printf("\n");
	int port, level, seed = -1;
	char* server;
	if(argc < 5) {
		printf("ERROR: Not enough arguments!\n");
		printf("Syntax: ./treasure_hunter server port level seed\n\n");
		exit(1);
	}
	server = argv[1];
	port = atoi(argv[2]);
	level = atoi(argv[3]);
	seed = atoi(argv[4]);

	printf("Server: %s \n", server);
	printf("port: %d \n", port);
	printf("level: %d \n", level);
	printf("seed: %d \n", seed);
	fflush(stdout);

	unsigned char message[8];
	/*
	* Byte 0 is 0 always,
	* Byte 1 is level as an int
	*/
	message[0] = 0x0;
	message[1] = level;

	/*
	* Bytes 2-5: USERID in hex, put into network order (big endian)
	*/
	unsigned int val = htonl(USERID); 
	memcpy(&message[2], &val, sizeof(unsigned int));

	/*
	* Bytes 6-7: Seed used for RNG in network order (big endian)
	*/
	unsigned short val2 = htons(seed);
	memcpy(&message[6], &val2, sizeof(unsigned short));	

	print_bytes(message, 8);


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
