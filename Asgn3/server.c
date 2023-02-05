/*
    Assignment Number: 3
    Name: Sarita Singh
    Roll No.: 20CS10053
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

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

void send_chunks(int new_socket, char *result)
{
    char buffersend[52];
    int res_len = strlen(result);
    result[res_len] = '\0';
    int count = 0;
    res_len++;
    for (int i = 0; i < res_len; i += 50)
    {
        count = 0;
        memset(&buffersend, '\0', 52);
        for (int j = 0; j < 50 && i + j < res_len; j++)
        {
            count++;
            buffersend[j] = result[i + j];
        }
        send(new_socket, buffersend, count, 0);
    }
}

int main(int argc, char *argv[])
{
    int server_socket;
    int clilen, error_check;
    if(argc <= 1){
        perror("\nServer Port is required\n");
        exit(0);
    }
    int SERVER_PORT = atoi(argv[1]);

    // tcp server
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Error in creating socket\n");
        exit(0);
    }

    struct sockaddr_in server_address, client_address;

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_address.sin_port = htons(SERVER_PORT);

    // bind the socket to our specified IP & port information in server_address
    error_check = bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address));
    if (error_check < 0)
    {
        perror("Error in binding");
        exit(0);
    }

    // listen for client sockets where 5 is the max no of allowed socket
    error_check = listen(server_socket, 5);
    if (error_check == -1)
        perror("Error in listening\n");

    // buffer to store the date and time
    char buf_date_time[50];
    time_t curr_time;
    srand(time(NULL)); // Initialization
    int new_socket;
    // It is an iterative server hence we perform an infinite loop and after every request we close the connection with the client.
    while (1)
    {   
        // here the client is load balancer
        clilen = sizeof(client_address);
        if ((new_socket = accept(server_socket, (struct sockaddr *)&client_address, &clilen)) < 0)
        {
            perror("Error in Accept\n");
            exit(0);
        }
        
        int x;
        char buf[50];
        memset(&buf, '\0', sizeof(buf));

        if ((x = receive_chunks(new_socket, buf, 50)) < 0)
        {
            perror("\nError in receiving\n");
            exit(0);
        }
        // if load balancer asks for load, send a random number in [1,100]
        if (strcmp(buf, "Send Load") == 0)
        {
            int rand_num = rand()%100 + 1;
            size_t load = htonl(rand_num);
            send(new_socket, &load, sizeof(load), 0);
            printf("\nLoad sent: %d\n", rand_num);
        }
        else if (strcmp(buf, "Send Time") == 0)
        {
            memset(&buf_date_time, '\0', sizeof(buf_date_time));
            curr_time = time(NULL);
  
            // ctime used to get string representing local time for the curr_time
            snprintf(buf_date_time, sizeof(buf_date_time), "\a%s\n", ctime(&curr_time));

            send(new_socket, buf_date_time + 5, strlen(buf_date_time) + 1, 0);
        }
        // we close the connection with the client
        close(new_socket);
    }
    close(server_socket);
    return 0;
}