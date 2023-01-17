/* 
    Assignment Number: 1
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

int main()
{
    int server_socket;
    int clilen, error_check;
    
    // tcp server
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Error in creating socket\n");
		exit(0);
	}
    struct sockaddr_in server_address, client_address;

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(20000);

    // bind the socket to our specified IP & port information in server_address
    error_check = bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address));
    if (error_check < 0) {
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

    int new_socket;
    // It is an iterative server hence we perform an infinite loop and after every request we close the connection with the client.
    while (1)
    {   
        clilen = sizeof(client_address);
        if ((new_socket = accept(server_socket, (struct sockaddr *) &client_address, &clilen))< 0) {
			perror("Error in Accept\n");
			exit(0);
		}

        curr_time = time(NULL);
        // ctime used to get string representing local time for the curr_time
        snprintf(buf_date_time, sizeof(buf_date_time), "\a%s\n", ctime(&curr_time));

        send(new_socket, buf_date_time+5, strlen(buf_date_time)+1, 0);

        // we close the connection with the client
        close(new_socket);
    
    }

    return 0;
}