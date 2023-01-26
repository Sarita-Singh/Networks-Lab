/*
    Assignment Number: 2
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

#define PORT 20000
#define COMMAND_SIZE 5000

// function to receive from server in chunks
int receivefromserver(int sockfd, char *result, int size)
{   
    int n, total=0;
    while(1){
        char temp[52];
        memset(&temp, '\0', 52);
        n = recv(sockfd,temp,50,0);
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
        total+=n;
        long long l = strlen(temp);
        if(l + n > size){
            size = l + n  + 1;
            result  = (char *)realloc(result,size);
        }
        strcat(result,temp);
        if(temp[n - 1] == '\0')
            break;
    }
    return total;
}

int main()
{
    int connection_socket;
    struct sockaddr_in server_address;

    // create client socket
    if ((connection_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("\nError in creating socket\n");
        exit(0);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    inet_aton("127.0.0.1", &server_address.sin_addr);

    // creating connection with server at specified address
    if ((connect(connection_socket, (struct sockaddr *)&server_address, sizeof(server_address))) < 0)
    {
        perror("\nError in connecting to server\n");
        exit(0);
    }

    int x;
    char ch, buf[50];
    memset(&buf, '\0', sizeof(buf));

    if ((x = receivefromserver(connection_socket, buf, 50)) < 0)
    {
        perror("\nError in receiving\n");
    }

    printf("\n%s\n", buf);
    
    char username[50];
    scanf("%[^\n]s", username);
    username[strlen(username)] = '\0';
    // send the username to the server
    if (send(connection_socket, username, strlen(username) + 1, 0) < 0)
    {
        perror("\nError in sending\n");
    }
    // printf("\nUsername sent to the server.\n");

    memset(&buf, '\0', sizeof(buf));

    // receive validation of username
    if ((x = receivefromserver(connection_socket, buf, sizeof(buf))) < 0)
    {
        perror("\nError in receiving\n");
    }
    if (strcmp(buf, "NOT-FOUND") == 0)
    {
        printf("\nInvalid username\n");
        close(connection_socket);
        exit(0);
    }
    ch = getchar();
    while (1)
    {
        char sh_cmd[COMMAND_SIZE];
        memset(&sh_cmd, '\0', COMMAND_SIZE);

        printf("\nEnter a shell command: ");
        long long c = 0;
        // loop to read command untill newline is entered
        do
        {
            ch = getchar();
            sh_cmd[c] = ch;
            c++;
        } while (ch != '\n');
        c = c - 1;
        sh_cmd[c] = '\0';

        // printf("%s %d\n", sh_cmd, c);

        // terminate if user enters the special command "exit"
        if (strcmp(sh_cmd, "exit") == 0)
        {
            close(connection_socket);
            exit(0);
        }

		int len = strlen(sh_cmd) + 1, cnt;
        // printf("strlen=%d", strlen(sh_cmd));
		for(int i = 0; i < len; i += 50){
			memset(&buf, '\0', 50);
			cnt = 0;
			for(int j = 0; j < 50 && i + j < len; j++){
				cnt++;
				buf[j] = sh_cmd[i+j];
			}
			send(connection_socket,buf,cnt,0);
		}
        // get the result after command is executed
        char *result = (char *)malloc(sizeof(char)*COMMAND_SIZE);
		strcpy(result,"\0");
		long long size = 2000;
        if ((x = receivefromserver(connection_socket, result, size)) < 0)
        {
            perror("\nError in receiving\n");
            exit(0);
        }
        if (strcmp(result, "$$$$") == 0)
        {
            printf("\nInvalid command\n");
        }
        else if (strcmp(result, "####") == 0)
        {
            printf("\nError in running command\n");
        }
        else
        {   
            printf("\nCommand output: \n%s\n", result);
        }
        free(result);
    }
    close(connection_socket);
    return 0;
}