/*
    Assignment Number: 2
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
#include <time.h>
#include <sys/poll.h>
#include <string.h>

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

int main(char argc, char *argv[])
{
    int lb_socket, server1_socket, server2_socket;
    int clilen, error_check;

    if (argc <= 3)
    {
        perror("\nLoad balancer and server ports are required\n");
        exit(0);
    }
    int LB_PORT, S1_PORT, S2_PORT;
    LB_PORT = atoi(argv[1]);
    S1_PORT = atoi(argv[2]);
    S2_PORT = atoi(argv[3]);

    if ((lb_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("\nError in creating load balancer socket\n");
        exit(0);
    }

    struct sockaddr_in lb_address, server1_address, server2_address, client_address;

    lb_address.sin_family = AF_INET;
    lb_address.sin_port = htons(LB_PORT);
    lb_address.sin_addr.s_addr = INADDR_ANY;

    server1_address.sin_family = AF_INET;
    server1_address.sin_port = htons(S1_PORT);
    inet_aton("127.0.0.1", &server1_address.sin_addr);

    server2_address.sin_family = AF_INET;
    server2_address.sin_port = htons(S2_PORT);
    inet_aton("127.0.0.1", &server2_address.sin_addr);

    // bind the socket to our specified IP & port information in server_address
    error_check = bind(lb_socket, (struct sockaddr *)&lb_address, sizeof(lb_address));
    if (error_check < 0)
    {
        perror("\nError in binding\n");
        exit(0);
    }
    // Up to 5 concurrent client requests will be queued up while the system is
    // executing the accept system call
    error_check = listen(lb_socket, 5);
    if (error_check == -1)
    {
        perror("\nError in listening\n");
        exit(0);
    }

    char buf[50]; // buffer used for communication
    memset(&buf, '\0', sizeof(buf));

    int x, client_socket;
    int s1_prev_load = 0, s2_prev_load = 0;
    struct pollfd fds[2];
    fds[0].fd = lb_socket;
    fds[0].events = POLLIN;
    int time_used = 0;
    while (1)
    {
        time_t start_time, current_time;
        // record the start time here
        time(&start_time);
        // printf("\nStarting poll with %d milli sec\n", 5000 - time_used);
        int ret = poll(fds, 2, 5000 - time_used);

        if (ret == 0)
        {
            time(&start_time);
            // receive loads from the two servers
            // creating connection with server at specified address
            if ((server1_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                perror("\nError in creating server 1 socket\n");
                exit(0);
            }
            if ((connect(server1_socket, (struct sockaddr *)&server1_address, sizeof(server1_address))) < 0)
            {
                perror("\nError in connecting to first server\n");
                exit(0);
            }
            memset(&buf, '\0', sizeof(buf));
            strcpy(buf, "Send Load");
            send_chunks(server1_socket, buf);
            size_t received_int = 0;
            if ((x = recv(server1_socket, &received_int, sizeof(received_int), 0)) < 0)
            {
                perror("\nError in receiving\n");
                exit(0);
            }
            s1_prev_load = ntohl(received_int);
            printf("\nLoad received from %d:%d %d\n", server1_address.sin_addr.s_addr, server1_address.sin_port, s1_prev_load);
            close(server1_socket);

            if ((server2_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                perror("\nError in creating server 2 socket\n");
                exit(0);
            }
            if ((connect(server2_socket, (struct sockaddr *)&server2_address, sizeof(server2_address))) < 0)
            {
                perror("\nError in connecting to second server\n");
                exit(0);
            }
            send_chunks(server2_socket, buf);
            received_int = 0;
            if ((x = recv(server2_socket, &received_int, sizeof(received_int), 0)) < 0)
            {
                perror("\nError in receiving\n");
                exit(0);
            }
            s2_prev_load = ntohl(received_int);
            printf("\nLoad received from %d:%d %d\n", server2_address.sin_addr.s_addr, server2_address.sin_port, s2_prev_load);
            close(server2_socket);

            time_used = 0;
        }
        else if (ret > 0)
        {
            // lb acts like a client for the servers 1 and 2
            // record the end time here and call poll with (5-elasped time)
            time(&current_time);
            time_used = (int)difftime(current_time, start_time) * 1000;
            // printf("\nTime used up: %d\n", time_used);

            // send the received string to client
            clilen = sizeof(client_address);
            // client socket stored in variable client_socket after accepting the connection
            if ((client_socket = accept(lb_socket, (struct sockaddr *)&client_address, &clilen)) < 0)
            {
                perror("\nError in accept\n");
                exit(0);
            }
            char buf_date_time[50];
            // child process for our concurrent server
            if (fork() == 0)
            {
                close(lb_socket);
                memset(&buf_date_time, '\0', 50);

                if (s1_prev_load < s2_prev_load)
                {
                    // choose server 1
                    if ((server1_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                    {
                        perror("\nError in creating server 1 socket\n");
                        exit(0);
                    }
                    if ((connect(server1_socket, (struct sockaddr *)&server1_address, sizeof(server1_address))) < 0)
                    {
                        perror("\nError in connecting to first server\n");
                        exit(0);
                    }
                    memset(&buf, '\0', sizeof(buf));
                    strcpy(buf, "Send Time");
                    send_chunks(server1_socket, buf);
                    printf("\nSending client request to %d:%d", server1_address.sin_addr.s_addr, server1_address.sin_port);

                    if ((x = receive_chunks(server1_socket, buf_date_time, sizeof(buf_date_time))) < 0)
                    {
                        perror("\nError in receiving\n");
                        exit(0);
                    }
                    // printf("\nString received from server 1 %s\n", buf_date_time);
                    close(server1_socket);
                }
                else
                {
                    // choose server 2
                    if ((server2_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                    {
                        perror("\nError in creating server 2 socket\n");
                        exit(0);
                    }
                    if ((connect(server2_socket, (struct sockaddr *)&server2_address, sizeof(server2_address))) < 0)
                    {
                        perror("\nError in connecting to second server\n");
                        exit(0);
                    }
                    memset(&buf, '\0', sizeof(buf));
                    strcpy(buf, "Send Time");
                    send_chunks(server2_socket, buf);
                    printf("\nSending client request to %d:%d", server2_address.sin_addr.s_addr, server2_address.sin_port);

                    if ((x = receive_chunks(server2_socket, buf_date_time, sizeof(buf_date_time))) < 0)
                    {
                        perror("\nError in receiving\n");
                        exit(0);
                    }
                    // printf("\nString received from server 2 %s\n", buf_date_time);
                    close(server2_socket);
                }

                send_chunks(client_socket, buf_date_time);

                close(client_socket);
                exit(0);
            }
            close(client_socket);
        }
        else
        {
            perror("\nError in poll\n");
        }
    }
    close(lb_socket);
    return 0;
}