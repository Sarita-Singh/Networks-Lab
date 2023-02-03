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
#include <poll.h>
#include <time.h>

			/* THE SERVER PROCESS */

int main(int argc, char* argv[])
{
    if(argc <= 3) {
        printf("Please provide the load balancer port, server1 port and server2 port respectively\n");
		exit(0);
    }

	int			sockfd, newsockfd, serv1_sockfd, serv2_sockfd ; /* Socket descriptors */
	int			clilen;
	struct sockaddr_in	cli_addr, load_addr, serv1_addr, serv2_addr;

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
	load_addr.sin_family		= AF_INET;
	load_addr.sin_addr.s_addr	= INADDR_ANY;
	load_addr.sin_port		= htons(atoi(argv[1]));

    serv1_addr.sin_family		= AF_INET;
	serv1_addr.sin_addr.s_addr	= INADDR_ANY;
	serv1_addr.sin_port		= htons(atoi(argv[2]));

    serv2_addr.sin_family		= AF_INET;
	serv2_addr.sin_addr.s_addr	= INADDR_ANY;
	serv2_addr.sin_port		= htons(atoi(argv[3]));

	/* With the information provided in load_addr, we associate the server
	   with its port using the bind() system call. 
	*/
	if (bind(sockfd, (struct sockaddr *) &load_addr,
					sizeof(load_addr)) < 0) {
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

    struct pollfd pfd;
    pfd.fd = sockfd;
    pfd.events = POLLIN;

	int timeDuration = 0;
	int server1Load = 0, server2Load = 0;

	while (1) {

		time_t timeStart, timeCurr;
		time(&timeStart);

		int pollResponse = poll(&pfd, 1, 5000 - timeDuration);

        if(pollResponse > 0) {

			time(&timeCurr);
			timeDuration = difftime(timeCurr, timeStart) * 1000;

            clilen = sizeof(cli_addr);
            newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
                        &clilen) ;

            if (newsockfd < 0) {
                printf("Accept error\n");
                exit(0);
            }

            if (fork() == 0) {


                close(sockfd);

				if(server1Load < server2Load) {

					if ((serv1_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
						printf("Cannot create socket for server\n");
						exit(0);
					}

					if ((connect(serv1_sockfd, (struct sockaddr *) &serv1_addr,
									sizeof(serv1_addr))) < 0) {
						perror("Unable to connect to server1\n");
						exit(0);
					}
					strcpy(buf,"Send Time");
					send(serv1_sockfd, buf, strlen(buf) + 1, 0);
					printf("Sending client request to %s\n", argv[2]);

					for(i=0; i < 100; i++) buf[i] = '\0';
					recv(serv1_sockfd, buf, 100, 0);

					close(serv1_sockfd);
				}
				else {
					if ((serv2_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
						printf("Cannot create socket for server\n");
						exit(0);
					}

					if ((connect(serv2_sockfd, (struct sockaddr *) &serv2_addr,
									sizeof(serv2_addr))) < 0) {
						perror("Unable to connect to server1\n");
						exit(0);
					}
					strcpy(buf,"Send Time");
					send(serv2_sockfd, buf, strlen(buf) + 1, 0);
					printf("Sending client request to %s\n", argv[3]);

					for(i=0; i < 100; i++) buf[i] = '\0';
					recv(serv2_sockfd, buf, 100, 0);

					close(serv2_sockfd);
				}
                
                send(newsockfd, buf, strlen(buf) + 1, 0);
                close(newsockfd);
                exit(0);
            }

            close(newsockfd);
        }
		else if(pollResponse == 0) {

			time(&timeStart);

			if ((serv1_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
				printf("Cannot create socket for server\n");
				exit(0);
			}

			if ((serv2_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
				printf("Cannot create socket for server\n");
				exit(0);
			}

			if ((connect(serv1_sockfd, (struct sockaddr *) &serv1_addr,
							sizeof(serv1_addr))) < 0) {
				perror("Unable to connect to server1\n");
				exit(0);
			}
			strcpy(buf,"Send Load");
			send(serv1_sockfd, buf, strlen(buf) + 1, 0);

			for(i=0; i < 100; i++) buf[i] = '\0';
			recv(serv1_sockfd, buf, 100, 0);
			printf("Load received from %s: %s\n", argv[2], buf);
			server1Load = atoi(buf);

			close(serv1_sockfd);

			if ((connect(serv2_sockfd, (struct sockaddr *) &serv2_addr,
							sizeof(serv2_addr))) < 0) {
				perror("Unable to connect to server2\n");
				exit(0);
			}
			strcpy(buf,"Send Load");
			send(serv2_sockfd, buf, strlen(buf) + 1, 0);

			for(i=0; i < 100; i++) buf[i] = '\0';
			recv(serv2_sockfd, buf, 100, 0);
			printf("Load received from %s: %s\n", argv[3], buf);
			server2Load = atoi(buf);

			close(serv2_sockfd);
		}
		else {
			printf("Error in poll\n");
		}
	}
	return 0;
}
			
