/*
    Assignment Number: 3
    Name: Sarita Singh
    Roll No.: 20CS10053
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define COMMAND_SIZE 5000

// function to receive from server in chunks
int receive_chunks(int sockfd, char *result, int size)
{
    int n, total = 0;
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
            size = l + n + 1;
            result = (char *)realloc(result, size);
        }
        strcat(result, temp);
        if (temp[n - 1] == '\0')
            break;
    }
    return total;
}

int main(int argc, char *argv[])
{
    int connection_socket;
    struct sockaddr_in server_address;
    if (argc <= 1)
    {
        perror("\nLoad balancer port is required\n");
        exit(0);
    }

    int LB_PORT = atoi(argv[1]);

    // create client socket
    if ((connection_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("\nError in creating socket\n");
        exit(0);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(LB_PORT);
    inet_aton("127.0.0.1", &server_address.sin_addr);

    // creating connection with server at specified address
    if ((connect(connection_socket, (struct sockaddr *)&server_address, sizeof(server_address))) < 0)
    {
        perror("\nError in connecting to server\n");
        exit(0);
    }
    
    int x;
    char buf[50];
    memset(&buf, '\0', sizeof(buf));

    if ((x = receive_chunks(connection_socket, buf, 50)) < 0)
    {
        perror("\nError in receiving\n");
        exit(0);
    }
    printf("\nServer Time: %s\n", buf);

    close(connection_socket);
    return 0;
}