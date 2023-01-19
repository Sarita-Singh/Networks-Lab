/*
    Assignment Number: 2
    Name: Sarita Singh
    Roll No.: 20CS10053
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <netinet/in.h>

#define PORT 20000 // The port number on which the server will be listening

int main()
{
    int connection_socket;

    // creating UDP socket for client
    if ((connection_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Error in creating socket\n");
        exit(0);
    }

    struct sockaddr_in server_address;
    // initializing server address to all zeros
    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET; // The internet family set to AF_INET
    server_address.sin_port = htons(PORT);
    // 127.0.0.1 is a special address for "localhost" (this machine)
    inet_aton("127.0.0.1", &server_address.sin_addr);

    // buffer to store date and time
    char buf_date_time[40];

    printf("\nHello User!\n");
    char cli_msg[20];
    strcpy(cli_msg, "CLIENT: HELLO");
    cli_msg[strlen(cli_msg)]='\0';

    sendto(connection_socket, (const char *)cli_msg, strlen(cli_msg), 0, (const struct sockaddr *)&server_address, sizeof(server_address));
    // printf("Hello message sent from client\n");

    struct pollfd fdset[2];
    fdset[0].fd = connection_socket;
    fdset[0].events = POLLIN;
    socklen_t l = sizeof(server_address);

    int trial_count = 1;
    int wait_time = 3000;
    while (trial_count <= 5)
    {
        // if poll() returns > 0 then receive the server time in buf_date_time
        if (poll(fdset, 2, wait_time) > 0)
        {
            int n;
            n = recvfrom(connection_socket, buf_date_time, 40, 0, (struct sockaddr *)&server_address, &l);
            if (n < 0)
            {
                perror("\nError in reading from socket\n");
                continue;
            }
            // printf("\n%d",n);
            buf_date_time[n] = '\0';
            printf("\nServer time: %s\n", buf_date_time);
            close(connection_socket);
            return 0;
        }
        trial_count++;
    }
    printf("\nTimeout exceeded\n");
    close(connection_socket);
    return 0;
}