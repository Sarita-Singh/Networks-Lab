/* 
    Assignment Number: 1
    Name: Sarita Singh
    Roll No.: 20CS10053
*/
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char **argv)
{
    int connection_socket;
    struct sockaddr_in server_address; 

    // creating a socket
    if((connection_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Error in creating socket\n");
        exit(0);
    }

    char address[16];

    if(argc >= 2) strcpy(address, argv[1]);
    else sprintf(address, "127.0.0.1");
    
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(20000); 
    inet_aton(address, &server_address.sin_addr);

    // creating connection with server at specified address
    if( connect(connection_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
       perror("\n Error in connecting to server \n");
       exit(0);
    } 

    char buf_data[5000];
    int x = 0;

    // get data from the server and display it
    if((x = recv(connection_socket, buf_data, sizeof(buf_data)-1,0)) < 0 )
    {
        perror("\n Error in receiving \n");
    } 
    printf("%s",buf_data);
    printf("\nsize received: %d\n", strlen(buf_data));
    
    return 0;
}