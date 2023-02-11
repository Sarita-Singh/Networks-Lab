#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define INITIAL_SIZE 512

char *receive_chunks(int sockfd)
{
    int n, total = 0;
    int size = INITIAL_SIZE;
    char *result = (char *)malloc(size * sizeof(char));
    while (1)
    {
        char temp[52];
        memset(&temp, '\0', 52);
        n = recv(sockfd, temp, 50, 0);
        if (n < 0)
        {
            perror("Unable to read from socket");
            exit(1);
        }
        if (n == 0)
        {
            perror("Connection closed by client");
            exit(1);
        }
        total += n;
        long long l = strlen(temp);
        if (l + n > size)
        {
            size = 2 * (l + n + 1);
            result = (char *)realloc(result, size);
        }
        strcat(result, temp);
        if (temp[n - 1] == '\0')
            break;
    }
    return result;
}

#define PORT 80
int main()
{
    int server_socket;
    int clilen, error_check;

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("\nError in creating socket\n");
        exit(0);
    }
    printf("\nSocket created\n");
    struct sockaddr_in server_address, client_address;

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // bind the socket to our specified IP & port information in server_address
    error_check = bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address));
    if (error_check < 0)
    {
        perror("\nError in binding\n");
        exit(0);
    }
    printf("\nBind done\n");
    // Up to 5 concurrent client requests will be queued up while the system is
    // executing the accept system call
    error_check = listen(server_socket, 5);
    if (error_check == -1)
    {
        perror("\nError in listening\n");
        exit(0);
    }
    printf("\nServer listening at port:%d\n", PORT);
    char buf[50]; // buffer used for communication
    char username[50];
    memset(&buf, '\0', sizeof(buf));
    memset(&username, '\0', sizeof(username));

    int i, newsockfd;
    while (1)
    {
        clilen = sizeof(client_address);

        // client socket stored in variable newsockfd after accepting the connection
        if ((newsockfd = accept(server_socket, (struct sockaddr *)&client_address, &clilen)) < 0)
        {
            perror("\nError in accept\n");
            exit(0);
        }
        printf("\nClient connected\n");
        char *result;
        result = receive_chunks(newsockfd);

        printf("\n%s", result);
        close(newsockfd);
    }
    return 0;
}