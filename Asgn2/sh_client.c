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

int receive(int sockfd, char *buf, int size, char delim)
{
    char temp[50];
    int total = 0;
    while (1)
    {
        memset(&temp, '\0', sizeof(temp));
        int n = recv(sockfd, temp, 50, 0);
        if (n < 0)
        {
            perror("Unable to read from socket");
            exit(1);
        }
        if (n == 0)
        {
            break;
        }
        strcat(buf, temp);
        total += n;
        if (temp[n - 1] == delim)
        {
            break;
        }
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

    if ((x = receive(connection_socket, buf, sizeof(buf), '\0')) < 0)
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
    printf("\nUsername sent to the server.\n");

    memset(&buf, '\0', sizeof(buf));

    // receive validation of username
    if ((x = receive(connection_socket, buf, sizeof(buf), '\0')) < 0)
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
        // loop to read characters untill newline is entered
        do
        {
            ch = getchar();
            sh_cmd[c] = ch;
            c++;
        } while (ch != '\n');
        c = c - 1;
        sh_cmd[c] = '\0';

        // printf("%s %d", sh_cmd, strlen(sh_cmd));

        // terminate if user enters the special command "exit"
        if (strcmp(sh_cmd, "exit") == 0)
        {
            close(connection_socket);
            exit(0);
        }

        int bufsize = 50;
        char cmd_buf[50];
		int len = strlen(sh_cmd) + 1;
		//send the command to the user in chunks
		for(int i = 0; i < len; i += bufsize){
			for(int j = 0; j < bufsize ; j++)
				cmd_buf[j] = '\0';
			int cnt = 0;
			for(int j = 0; j < bufsize && i + j < len; j++){
				cnt++;
				cmd_buf[j] = sh_cmd[i+j];
			}
			send(connection_socket,cmd_buf,cnt,0);
		}
        // get the result after command is executed
        char result[COMMAND_SIZE];
        memset(&result, '\0', COMMAND_SIZE);
        if ((x = receive(connection_socket, result, COMMAND_SIZE, '\0')) < 0)
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
        {   if(result[0]!='\0')
            printf("\nCommand output: %s\n", result);
        }
    }
    close(connection_socket);
    return 0;
}