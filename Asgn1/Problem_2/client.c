/*
    Assignment Number: 1
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
#include <math.h>
#include <string.h>

int main()
{
    int connection_socket;
    struct sockaddr_in server_address;
    while (1)
    {   
        // create client socket
        if ((connection_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            perror("\nError in creating socket\n");
            exit(0);
        }

        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(20000);
        inet_aton("127.0.0.1", &server_address.sin_addr);

        // creating connection with server at specified address
        if ((connect(connection_socket, (struct sockaddr *)&server_address, sizeof(server_address))) < 0)
        {
            perror("\nError in connecting to server\n");
            exit(0);
        }
        printf("\nEnter the expression (Enter -1 to terminate and newline to end the expression): ");

        int max = 512;
        char *buf_data = (char *)malloc(sizeof(char) * max);
        if (buf_data == 0)
            perror("Memory exhausted");

        char ch;
        int c = 0;
        // loop to read characters untill newline is entered
        do
        {
            ch = getchar();
            if(ch==' ')
            continue;
            buf_data[c] = ch;
            c++;
            if (c == max - 1)
            {
                max *= 2;
                buf_data = (char *)realloc(buf_data, max);
                if (buf_data == 0)
                    perror("Memory exhausted");
            }
        } while (ch != '\n');
        c = c - 1;
        buf_data[c] = '\0';

        // if user enters "-1" then terminate the program
        if (strcmp(buf_data, "-1") == 0)
            break;

        // send the expression to the server
        if (send(connection_socket, buf_data, strlen(buf_data) + 1, 0) < 0)
        {
            perror("\nError in sending\n");
        }
        printf("\nExpression sent to the server.\n");

        int x = 0;
        float result;
        // get result from the server and display it
        if ((x = recv(connection_socket, &result, sizeof(result), 0)) < 0)
        {
            perror("\nError in receiving\n");
        }
        if (result == INFINITY)
            printf("\nResult received from server: INFINITY\n");
        else
            printf("\nResult received from server: %f\n", result);

        close(connection_socket);
    }
    return 0;
}