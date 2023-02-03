// A Simple Client Implementation
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <poll.h>

#define MAXLINE 1024 
  
int main() { 
    int sockfd; 
    struct sockaddr_in servaddr; 
  
    // Creating socket file descriptor 
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if ( sockfd < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
  
    memset(&servaddr, 0, sizeof(servaddr)); 
      
    // Server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(8181); 
    inet_aton("127.0.0.1", &servaddr.sin_addr); 
      
    int n;
    socklen_t len; 
    char *hello = "CLIENT:HELLO";

    sendto(sockfd, (const char *)hello, strlen(hello), 0, 
			(const struct sockaddr *) &servaddr, sizeof(servaddr)); 
    //printf("Hello message sent from client\n");

    struct pollfd pfd;
    pfd.fd = sockfd;
    pfd.events = POLLIN;

    int trialsLeft = 5;

    while(trialsLeft-- > 0) {

        if(poll(&pfd, 1, 3000) > 0) {

            len = sizeof(servaddr);
            char buffer[MAXLINE]; 
            n = recvfrom(sockfd, (char *)buffer, MAXLINE, 0, 
                    ( struct sockaddr *) &servaddr, &len);
            buffer[n] = '\0'; 
            printf("%s\n", buffer);
                
            close(sockfd); 
            return 0; 
        }

        printf("Retrying...\n");
    }

    printf("Timeout Exceeded\n");
    close(sockfd); 
    return 0; 
} 