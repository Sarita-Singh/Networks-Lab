
/*    THE CLIENT PROCESS */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int recvChunk(int sockfd, char *response, int responseSize) {

	int tempSize, totalSize = 0;
	char tempBuffer[50];

	while(1) {
		for(int i=0; i < 50; i++) tempBuffer[i] = '\0';
		tempSize = recv(sockfd, tempBuffer, 50, 0);
		if(tempSize < 0) {
			printf("Failed to receive data chunk\n");
			exit(0);
		}
		else if(tempSize == 0) break;
		else {
			totalSize += tempSize;
			int tempStringLen = strlen(tempBuffer);
			if(totalSize + tempSize > responseSize) {
				responseSize = totalSize + tempSize + 1;
				response = (char *)realloc(response, responseSize);
			}
			strcat(response, tempBuffer);
			if(tempBuffer[tempSize-1] == '\0') break;
		}
	}
	return totalSize;
}

int main()
{
	int			sockfd ;
	struct sockaddr_in	serv_addr;

	int i;
	char buf[50];

	/* Opening a socket is exactly similar to the server process */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to create socket\n");
		exit(0);
	}

	/* Recall that we specified INADDR_ANY when we specified the server
	   address in the server. Since the client can run on a different
	   machine, we must specify the IP address of the server. 

	   In this program, we assume that the server is running on the
	   same machine as the client. 127.0.0.1 is a special address
	   for "localhost" (this machine)
	   
	/* IF YOUR SERVER RUNS ON SOME OTHER MACHINE, YOU MUST CHANGE 
           THE IP ADDRESS SPECIFIED BELOW TO THE IP ADDRESS OF THE 
           MACHINE WHERE YOU ARE RUNNING THE SERVER. 
    	*/

	serv_addr.sin_family	= AF_INET;
	inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port	= htons(20000);

	/* With the information specified in serv_addr, the connect()
	   system call establishes a connection with the server process.
	*/
	if ((connect(sockfd, (struct sockaddr *) &serv_addr,
						sizeof(serv_addr))) < 0) {
		perror("Unable to connect to server\n");
		exit(0);
	}

	/* After connection, the client can send or receive messages.
	   However, please note that recv() will block when the
	   server is not sending and vice versa. Similarly send() will
	   block when the server is not receiving and vice versa. For
	   non-blocking modes, refer to the online man pages.
	*/
	for(i=0; i < 50; i++) buf[i] = '\0';
	recv(sockfd, buf, 50, 0);
	printf("%s", buf);

    char username[26];
    scanf("%s", &username);
    username[25] = '\0';
    send(sockfd, username, strlen(username) + 1, 0);

    for(i=0; i < 50; i++) buf[i] = '\0';
	recv(sockfd, buf, 50, 0);
	
    if(strcmp(buf, "NOT-FOUND") == 0) {
        printf("Invalid username\n");
    }
    else if(strcmp(buf, "FOUND") == 0) {
        getchar();
        while(1) {
            printf("Enter commmand to execute: ");
            char commandBuf[500];
            char* responseBuf = (char *)malloc(500*sizeof(char));

            char ch = getchar();
            int temp = 0;
            while(ch != '\n') {
                commandBuf[temp] = ch;
                temp++;
                ch = getchar();
            }
            commandBuf[temp] = '\0';

            int commandLen = strlen(commandBuf)+1;
            
            for(int i = 0; i < commandLen; i+=50) {
                for(int k=0; k < 50; k++) buf[k] = '\0';
                int temp = 0;
                for(int j = 0; j < 50 && i + j < commandLen; j++) {
                    buf[j] = commandBuf[i+j];
                    temp++;
                }
                send(sockfd, buf, temp, 0);
            }

            if(strcmp(commandBuf, "exit") == 0) break;
            else {
                for(i=0; i < 500; i++) responseBuf[i] = '\0';
                recvChunk(sockfd, responseBuf, 500);
                
                if(strcmp(responseBuf, "####") == 0) {
                    printf("Error in running command\n");
                }
                else if(strcmp(responseBuf, "$$$$") == 0) {
                    printf("Invalid command\n");
                }
                else if(responseBuf[0] != '\0'){
                    printf("Response from server: %s\n", responseBuf);
                }
            }
        }
    }
    else {
        printf("Invalid response from server\n");
    }
		
	close(sockfd);
	return 0;

}
