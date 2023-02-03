/*
			NETWORK PROGRAMMING WITH SOCKETS

In this program we illustrate the use of Berkeley sockets for interprocess
communication across the network. We show the communication between a server
process and a client process.


*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>

			/* THE SERVER PROCESS */

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

void sendChunk(int sockfd, char* response, int responseSize) {
	char buf[50];
	for(int i = 0; i < responseSize; i+=50) {
		for(int k=0; k < 50; k++) buf[k] = '\0';
		int temp = 0;
		for(int j = 0; j < 50 && i + j < responseSize; j++) {
			buf[j] = response[i+j];
			temp++;
		}
		send(sockfd, buf, temp, 0);
	}
}

void senCmdErrSignal(int sockfd) {
	send(sockfd, "####", 5, 0);
}

int main()
{
	int			sockfd, newsockfd ; /* Socket descriptors */
	int			clilen;
	struct sockaddr_in	cli_addr, serv_addr;

	int i;
	char buf[100];		/* We will use this buffer for communication */

	/* The following system call opens a socket. The first parameter
	   indicates the family of the protocol to be followed. For internet
	   protocols we use AF_INET. For TCP sockets the second parameter
	   is SOCK_STREAM. The third parameter is set to 0 for user
	   applications.
	*/
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Cannot create socket\n");
		exit(0);
	}

	/* The structure "sockaddr_in" is defined in <netinet/in.h> for the
	   internet family of protocols. This has three main fields. The
 	   field "sin_family" specifies the family and is therefore AF_INET
	   for the internet family. The field "sin_addr" specifies the
	   internet address of the server. This field is set to INADDR_ANY
	   for machines having a single IP address. The field "sin_port"
	   specifies the port number of the server.
	*/
	serv_addr.sin_family		= AF_INET;
	serv_addr.sin_addr.s_addr	= INADDR_ANY;
	serv_addr.sin_port		= htons(20000);

	/* With the information provided in serv_addr, we associate the server
	   with its port using the bind() system call. 
	*/
	if (bind(sockfd, (struct sockaddr *) &serv_addr,
					sizeof(serv_addr)) < 0) {
		printf("Unable to bind local address\n");
		exit(0);
	}

	listen(sockfd, 5); /* This specifies that up to 5 concurrent client
			      requests will be queued up while the system is
			      executing the "accept" system call below.
			   */

	/* In this program we are illustrating a concurrent server -- one
	   which forks to accept multiple client connections concurrently.
	   As soon as the server accepts a connection from a client, it
	   forks a child which communicates with the client, while the
	   parent becomes free to accept a new connection. To facilitate
	   this, the accept() system call returns a new socket descriptor
	   which can be used by the child. The parent continues with the
	   original socket descriptor.
	*/
	while (1) {

		/* The accept() system call accepts a client connection.
		   It blocks the server until a client request comes.

		   The accept() system call fills up the client's details
		   in a struct sockaddr which is passed as a parameter.
		   The length of the structure is noted in clilen. Note
		   that the new socket descriptor returned by the accept()
		   system call is stored in "newsockfd".
		*/
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
					&clilen) ;

		if (newsockfd < 0) {
			printf("Accept error\n");
			exit(0);
		}

		/* Having successfully accepted a client connection, the
		   server now forks. The parent closes the new socket
		   descriptor and loops back to accept the next connection.
		*/
		if (fork() == 0) {

			/* This child process will now communicate with the
			   client through the send() and recv() system calls.
			*/
			close(sockfd);	/* Close the old socket since all
					   communications will be through
					   the new socket.
					*/

			/* We initialize the buffer, copy the message to it,
			   and send the message to the client. 
			*/
			
			strcpy(buf,"LOGIN:");
			send(newsockfd, buf, strlen(buf) + 1, 0);

			/* We again initialize the buffer, and receive a 
			   message from the client. 
			*/
			for(i=0; i < 100; i++) buf[i] = '\0';
			recv(newsockfd, buf, 100, 0);

			FILE* fp = fopen("users.txt", "r");
			char fileBuf[26];

			int found = 0;

			while(fgets(fileBuf, 26, fp)) {
				fileBuf[strcspn(fileBuf, "\n")] = 0;
				if(strcmp(buf, fileBuf) == 0) found = 1;
			}

			fclose(fp);

			if(found == 0) {
				strcpy(buf,"NOT-FOUND");
				send(newsockfd, buf, strlen(buf) + 1, 0);
				close(newsockfd);
				exit(0);
			} 
			else {
				strcpy(buf,"FOUND");
				send(newsockfd, buf, strlen(buf) + 1, 0);

				char *commandBuf = (char *)malloc(500*sizeof(char));
				if(commandBuf == 0) {
					printf("No free memory left. Exiting.\n");
					exit(0);
				}

				char *resultBuf = (char *)malloc(500*sizeof(char));
				if(resultBuf == 0) {
					printf("No free memory left. Exiting.\n");
					exit(0);
				}

				while(1) {
					for(i=0; i < 500; i++) commandBuf[i] = '\0';
					recvChunk(newsockfd, commandBuf, 500);

					for(int i=0; i < 500; i++) resultBuf[i] = '\0';
					
					if(strcmp(commandBuf, "exit") == 0) break;
					else if(strcmp(commandBuf, "pwd") == 0) {
						char temp[500];
						char* status = getcwd(temp, sizeof(temp));
						strcpy(resultBuf,temp);
						if(status == NULL) senCmdErrSignal(newsockfd);
						else sendChunk(newsockfd, resultBuf, strlen(resultBuf)+1);
					}
					else if(commandBuf[0] == 'c' && commandBuf[1] == 'd' && (commandBuf[2] == '\0' || commandBuf[2] == ' ')) {
						char temp[500];
						int i = 2, j = 0;
						while(commandBuf[i] == ' ') i++;
						for(; commandBuf[i] != '\0'; i++, j++) temp[j] = commandBuf[i];
						temp[j] = '\0';
						if(j == 0) strcpy(temp, getenv("HOME"));
						int status = chdir(temp);
						if(status < 0) senCmdErrSignal(newsockfd);
						sendChunk(newsockfd, resultBuf, strlen(resultBuf)+1);
					}
					else if(commandBuf[0] == 'd' && commandBuf[1] == 'i' && commandBuf[2] == 'r' && (commandBuf[3] == '\0' || commandBuf[3] == ' ')) {
						char temp[500];
						int i = 3, j = 0;
						while(commandBuf[i] == ' ') i++;
						for(; commandBuf[i] != '\0'; i++, j++) temp[j] = commandBuf[i];
						temp[j] = '\0';
						if(j == 0) strcpy(temp, ".");
						DIR* dr = opendir(temp);
						if(dr == NULL) senCmdErrSignal(newsockfd);
						struct dirent *de;
						strcat(resultBuf, "\n");
						while ((de = readdir(dr)) != NULL) {
							strcat(resultBuf, de->d_name);
							strcat(resultBuf, "\n");
						}
						sendChunk(newsockfd, resultBuf, strlen(resultBuf)+1);
					}
					else send(newsockfd, "$$$$", 5, 0);
				}
			}

			close(newsockfd);
			exit(0);
		}

		close(newsockfd);
	}
	return 0;
}
			

