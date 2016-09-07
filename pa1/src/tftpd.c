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

int main()
{
	int isopen = 0;
	int blocknum;
	char buf[512];
	int filedesc;
    int sockfd;
    struct sockaddr_in server, client;
    char message[512];
	char* filename;
	struct datapack{
		uint16_t opcode;
		uint16_t blockno;
		char payload[BLOCK_SIZE];
	};
	
	struct datapack pack;
	
    /* Create and bind a UDP socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;

    /* Network functions need arguments in network byte order instead
     * of host byte order. The macros htonl, htons convert the
     * values.
     */
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(7329);
    bind(sockfd, (struct sockaddr *) &server, (socklen_t) sizeof(server));

    for (;;) {
        /* Receive up to one byte less than declared, because it will
         * be NUL-terminated later.
         */
				fflush(stdout);
		 
        socklen_t len = (socklen_t) sizeof(client);
        recvfrom(sockfd, message, sizeof(message) - 1,
                             0, (struct sockaddr *) &client, &len);
		
		//filedesc = open("testfile.txt", O_WRONLY | O_APPEND);
		if (message[1] == 1)
		{
			
			char* path = "../data/";
			filename = &message[2];
			//printf("%s", filename);
			char * filenamewpath = malloc(strlen(filename) + 1 + strlen(path));
			filenamewpath[0] = '\0';
			strcat(filenamewpath, path);
			strcat(filenamewpath, filename);
			//printf("%s \n", filenamewpath);
			filedesc = open(filenamewpath, O_RDONLY);
			isopen = 1;
			blocknum = 0;
		}
		else if (message[1] == 4)
		{
			printf("message 4");
		}
		
		// if filedescriptor is open
		if (isopen)
		{
			blocknum++;
			int packsize = read(filedesc, buf, BLOCK_SIZE);
			printf("filingur1");
			memcpy(pack.payload, buf, packsize);
			printf("filingur2");
			
			pack.opcode = htons(3);
			pack.blockno = htons(blocknum);
			packsize += 4;
			
			if(packsize < 516)
			{
				close(filedesc);
				isopen = 0;
			}
			sendto(sockfd, &pack, (size_t)packsize, 0,
            (struct sockaddr *) &client, len);
		}
		fflush(stdout);
    }
}
