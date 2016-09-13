#include <assert.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>

#define BLOCK_SIZE 512

int main(int argc, char**argv)
{
	int packsize;
	int isopen = 0;
	int blocknum = 0;
	char buf[512];
	int filedesc;
    int sockfd;
    struct sockaddr_in server, client, clientcheck;
    char message[512];
	char* filename;
	struct datapack{
		uint16_t opcode;
		uint16_t blockno;
		char payload[BLOCK_SIZE];
	};
	
	struct errorpack{
		uint16_t opcode;
		uint16_t errorcode;
		char error_message[BLOCK_SIZE];
	};
	
	struct datapack pack;
	struct errorpack errpack;
	
	if(argc < 2) {
		printf("You must supply a port number to run the server");
		exit(0);
	}
	
    /* Create and bind a UDP socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;

    /* Network functions need arguments in network byte order instead
     * of host byte order. The macros htonl, htons convert the
     * values.
     */
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(atoi(argv[1]));
    bind(sockfd, (struct sockaddr *) &server, (socklen_t) sizeof(server));

    for (;;) {
		
		fd_set rfds;
        struct timeval tv;
        int retval;

        /* Check whether there is data on the socket fd. */
		FD_ZERO(&rfds);
        FD_SET(sockfd, &rfds);

                /* Wait for five seconds. */
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        retval = select(sockfd + 1, &rfds, NULL, NULL, &tv);
		
        if (retval == -1) {
            perror("select()");
            } else if (retval > 0) {
             /* Data is available, receive it. */		
		
        socklen_t len = (socklen_t) sizeof(client);
        recvfrom(sockfd, message, sizeof(message) - 1,
                             0, (struct sockaddr *) &client, &len);
		
		if (message[1] == 1)
		{
			fflush(stdout);
			// copy client info so we it can
			// be verified later
			clientcheck.sin_port = client.sin_port;
			clientcheck.sin_addr = client.sin_addr;
			char* path = argv[2];
			filename = &message[2];
			char * filenamewpath = malloc(strlen(filename) + strlen(path) + 1);
			filenamewpath[0] = '\0';
			strcat(filenamewpath, path);
			strcat(filenamewpath, filename);
			filedesc = open(filenamewpath, O_RDONLY);
			if(filedesc < 0)
			{
				errpack.opcode = htons(5);
				errpack.errorcode = htons(1);
				sprintf(errpack.error_message, "file not found!");
				int textlen = strlen("file not found!");
				int errpacksize = textlen + 5;
				errpack.error_message[textlen] = '\0';
				sendto(sockfd, &errpack, (size_t)errpacksize, 0,
				(struct sockaddr *) &client, len);	
				continue;
			}
			else
			{
				isopen = 1;
				blocknum = 0;
			}
		}
		// writing should not be allowed
		else if(message[1] == 2)
		{
			errpack.opcode = htons(5);
			errpack.errorcode = htons(2);
			sprintf(errpack.error_message, "writing not allowed!");
			int textlen = strlen("writing not allowed!");
			int errpacksize = textlen + 5;
			errpack.error_message[textlen] = '\0';
			sendto(sockfd, &errpack, (size_t)errpacksize, 0,
				(struct sockaddr *) &client, len);	
			continue;
		}
		// uploading should not be allowed
		else if(message[1] == 3)
		{
			errpack.opcode = htons(5);
			errpack.errorcode = htons(2);
			sprintf(errpack.error_message, "writing not allowed!");
			int textlen = strlen("uploading not allowed!");
			int errpacksize = textlen + 5;
			errpack.error_message[textlen] = '\0';
			sendto(sockfd, &errpack, (size_t)errpacksize, 0,
				(struct sockaddr *) &client, len);
			continue;
		}
		else if(message[1] == 4) {
			printf("message er 4");
			// Check if the client is the same one that made the RRQ.
			if(client.sin_addr.s_addr != clientcheck.sin_addr.s_addr 
			|| client.sin_port != clientcheck.sin_port) {
				printf("message er 4");
				errpack.opcode = htons(5);
				errpack.errorcode = htons(2);
				sprintf(errpack.error_message, "Access violation.");
				int textlen = strlen("Access violation.");
				int errpacksize = textlen + 5;
				errpack.error_message[textlen] = '\0';
				sendto(sockfd, &errpack, (size_t)errpacksize, 0,
					(struct sockaddr *) &client, len);
				continue;
			}
		}
		// if filedescriptor is open
			if (isopen)
			{
				blocknum++;
				if((packsize = read(filedesc, buf, BLOCK_SIZE)) < 0) {
					if(isopen) {
						printf("Error: failed to read %s\n", filename);
					}
				}
				memcpy(pack.payload, buf, packsize);
				
				pack.opcode = htons(3);
				pack.blockno = htons(blocknum);
				packsize += 4;
				
				if(packsize < 516)
				{
					if(close(filedesc) < 0)
					{
						printf("error closing file descriptor");
					}
					isopen = 0;
				}
				sendto(sockfd, &pack, (size_t)packsize, 0,
				(struct sockaddr *) &client, len);
			}
		}
		else {
            fprintf(stdout, "No message in five seconds.\n");
            fflush(stdout);
        }
		fflush(stdout);
    }
}
