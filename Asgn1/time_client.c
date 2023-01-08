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

int main()
{
    int connection_socket;
    struct sockaddr_in server_address; 

    // creating a socket
    if((connection_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Error in creating socket\n");
        exit(0);
    } 
    
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(7000); 
    inet_aton("127.0.0.1", &server_address.sin_addr);

    if( connect(connection_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
       perror("\n Error in connecting to server \n");
       exit(0);
    } 

    char buf_data[1000];
    int x = 0;
    if((x = recv(connection_socket, buf_data, sizeof(buf_data)-1,0)) < 0 )
    {
        perror("\n Error in receiving \n");
    } 
    printf("%s",buf_data);
    
    return 0;
}