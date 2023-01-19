/*
    Assignment Number: 2
    Name: Sarita Singh
    Roll No.: 20CS10053
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <netinet/in.h>

#define PORT 20000 // Port number

int main()
{
    int server_socket; // Socket file descriptor
    socklen_t clilen, error_check;

    // Create a UDP socket for the server, a negative return value indicates an error
    if ((server_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("\nError in creating socket\n");
        exit(0);
    }
    struct sockaddr_in client_address, server_address;

    // setting the struct server address and client address to all zeros
    memset(&server_address, 0, sizeof(server_address));
    memset(&client_address, 0, sizeof(client_address));

    server_address.sin_family = AF_INET;         // AF_INET used for the internet family
    server_address.sin_addr.s_addr = INADDR_ANY; // accept conections from any IP address like wifi or lan
    server_address.sin_port = htons(PORT);       // port number converted to network byte order

    // bind the socket to our specified IP & port information in server_address
    error_check = bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0;
    if (error_check < 0)
    {
        perror("\nError in binding\n");
        exit(0);
    }

    int n;
    printf("\nServer waiting for client connection...\n");
    char buf[40];
    memset(&buf, '\0', sizeof(buf));

    while (1)
    {
        clilen = sizeof(client_address);

        // Reset buffer everytime
        memset(buf, '\0', sizeof(buf));
        n = recvfrom(server_socket, buf, sizeof(buf), 0, (struct sockaddr *)&client_address, &clilen);
        if (n < 0)
        {
            perror("\nError in reading from socket\n");
            break;
        }
        
        printf("\nClient connected\n");
        n = 0;

        // buffer to store the date and time
        char buf_date_time[40];
        memset(&buf_date_time, '\0', sizeof(buf_date_time));
        time_t curr_time;

        curr_time = time(NULL);
        // ctime used to get string representing local time for the curr_time
        snprintf(buf_date_time, sizeof(buf_date_time), "\a%s\n", ctime(&curr_time));

        if ((n = sendto(server_socket, buf_date_time + 5, strlen(buf_date_time) + 1, 0, (struct sockaddr *)&client_address, sizeof(client_address))) < 0)
        {
            perror("\nError in sending to socket\n");
            exit(0);
        }   
    }

    close(server_socket); // close the socket
    return 0;
}