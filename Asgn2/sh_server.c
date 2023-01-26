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
#include <dirent.h>
#include <string.h>

#define PORT 20000
#define COMMAND_SIZE 5000

//function to check if username is present in file
int check_usrname(char *username)
{
    FILE *fp = fopen("users.txt", "r");
    if (fp == NULL)
    {
        printf("\nFile not opened!\n");
        exit(0);
    }
    char temp[25];
    memset(&temp, '\0', sizeof(temp));

    while (fscanf(fp, " %[^\n]\n", temp) != EOF)
    {
        if (!strcmp(temp, username))
        {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

int receivefromclient(int sockfd, char *result, int size)
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

// helper function for getting results of pwd command
int pwd_cmd(char *sh_cmd, char *result)
{
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL)
        return 0;
    strcpy(result, cwd);
    printf("Current working dir: %s\n", result);

    return 1;
}

// helper function for cd command
int cd_cmd(char *sh_cmd, long long cmd_len)
{
    char changedir[1024] = "\0";
    long long idx = 2;
    while (sh_cmd[idx] == ' ')
    {   
        if(idx >= cmd_len)
        break;
        idx++;
    }
    if (idx >= cmd_len)
    {   
        // get path of HOME
        char *temp = getenv("HOME");
        strcpy(changedir, temp);
    }
    else
    {
        strcat(changedir, sh_cmd + idx);
        if (sh_cmd[cmd_len - 1] != '/')
            strcat(changedir, "/");
    }
    if (chdir(changedir) < 0)
    {
        return 0;
    }
    return 1;
}

// helper function for getting results of dir command
int dir_cmd(char *sh_cmd, char *result, long long cmd_len)
{
    char currrent_dir[2096];
    long long idx = 3, temp_len = 0;
    while (sh_cmd[idx] == ' ')
    {
        if(idx >= cmd_len)
        break;
        idx++;
    }

    if (idx >= cmd_len)
    {
        getcwd(currrent_dir, sizeof(currrent_dir));
    }
    else
    {
        strcat(currrent_dir, sh_cmd + idx);
        if (sh_cmd[cmd_len - 1] != '/')
            strcat(currrent_dir, "/");
    }
    DIR *dir = opendir(currrent_dir);
    if (dir != NULL)
    {
        struct dirent *entry_dir;

        while ((entry_dir = readdir(dir)) != NULL)
        {
            long long len = strlen(entry_dir->d_name);
            long long result_len = strlen(result);
            if (len + result_len + 1 >= temp_len)
            {
                temp_len = len + result_len + 2;
                result = (char *)realloc(result, temp_len);
            }
            strcat(result, entry_dir->d_name);
            strcat(result, "\n");
        }
        if (strlen(result) > 0)
        {
            result[strlen(result) - 1] = '\0';
        }
    }
    else
    {
        perror("\nDirectory could not be opened\n");
        return 0;
    }

    closedir(dir);
    return 1;
}

void sendtoclient(int new_socket, char *result)
{   
    char buffersend[52];
    int res_len = strlen(result);
    result[res_len] = '\0';
    int count=0;
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

int main()
{
    int server_socket;
    int clilen, error_check;

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("\nError in creating socket\n");
        exit(0);
    }
    struct sockaddr_in server_address, client_address;

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // bind the socket to our specified IP & port information in server_address
    error_check = bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address));
    if (error_check < 0)
    {
        perror("\nError in binding\n");
        exit(0);
    }
    // Up to 5 concurrent client requests will be queued up while the system is
    // executing the accept system call
    error_check = listen(server_socket, 5);
    if (error_check == -1)
    {
        perror("\nError in listening\n");
        exit(0);
    }

    char buf[50]; // buffer used for communication
    char username[50];
    memset(&buf, '\0', sizeof(buf));
    memset(&username, '\0', sizeof(username));

    int i, new_socket;
    while (1)
    {
        clilen = sizeof(client_address);

        // client socket stored in variable new_socket after accepting the connection
        if ((new_socket = accept(server_socket, (struct sockaddr *)&client_address, &clilen)) < 0)
        {
            perror("\nError in accept\n");
            exit(0);
        }

        // child process for our concurrent server
        if (fork() == 0)
        {
            close(server_socket);

            strcpy(buf, "LOGIN:");
            buf[strlen(buf)] = '\0';
            // sending the string "LOGIN:"
            int n = send(new_socket, buf, strlen(buf) + 1, 0);
            if (n == 0)
            {
                perror("\nClient disconnected.\n");
                break;
            }
            memset(&username, '\0', 50);
            receivefromclient(new_socket, username, 50);

            printf("\nUsername = %s\n", username);
            // function call for username validation
            int valid = check_usrname(username);
            if (valid == 1)
            {
                char temp[7];
                strcpy(temp, "FOUND");
                temp[6] = '\0';
                send(new_socket, temp, strlen(temp) + 1, 0);
            }
            else
            {
                char temp[11];
                strcpy(temp, "NOT-FOUND");
                temp[10] = '\0';
                send(new_socket, temp, strlen(temp) + 1, 0);
                close(new_socket);
                exit(0);
            }
            while (1)
            {
                char *sh_cmd = (char *)malloc(sizeof(char)*COMMAND_SIZE);
                if (sh_cmd == 0)
                {
                    perror("Memory exhausted");
                    exit(0);
                }
                memset(sh_cmd, '\0', sizeof(sh_cmd));
                long long szres = COMMAND_SIZE;
                // receive the shell command in chunks
                receivefromclient(new_socket, sh_cmd, COMMAND_SIZE);

                // printf("%s", sh_cmd);
                // terminate if command is exit
                if (strcmp(sh_cmd, "exit") == 0)
                {
                    close(new_socket);
                    exit(0);
                }
                // execute the shell command
                char *result = (char *)malloc(sizeof(char) * COMMAND_SIZE);
                if (result == 0)
                {
                    perror("Memory exhausted");
                    exit(0);
                }
                memset(result, '\0', sizeof(result));
                char cmdrun_err[5], invalid_cmd[5];
                strcpy(cmdrun_err, "####");
                strcpy(invalid_cmd, "$$$$");
                int flag=0;
                long long cmd_len = strlen(sh_cmd);
                if (strcmp(sh_cmd, "pwd") == 0)
                {
                    int ret = pwd_cmd(sh_cmd, result);
                    if (ret == 0)
                    {
                        send(new_socket, cmdrun_err, sizeof(cmdrun_err), 0);flag=1;
                    }
                }
                else if (sh_cmd[0] == 'd' && sh_cmd[1] == 'i' && sh_cmd[2]=='r' && (sh_cmd[3]=='\0' || sh_cmd[3]==' '))
                {
                    int ret = dir_cmd(sh_cmd, result, cmd_len);
                    if (ret == 0)
                    {   
                        send(new_socket, cmdrun_err, sizeof(cmdrun_err), 0);flag=1;
                    }
                }
                else if (sh_cmd[0] == 'c' && sh_cmd[1] == 'd' && (sh_cmd[2] == '\0' || sh_cmd[2]== ' '))
                {   
                    int ret = cd_cmd(sh_cmd, cmd_len);
                    if (ret == 0)
                    {
                        send(new_socket, cmdrun_err, sizeof(cmdrun_err), 0);flag=1;
                    }
                }
                else
                {
                    send(new_socket, invalid_cmd, sizeof(invalid_cmd), 0);flag=1;
                }
                if(flag==0){
                sendtoclient(new_socket, result);
                }
                free(result);
                free(sh_cmd);
            }

            close(new_socket);
            exit(0);
        }
        close(new_socket);
    }
    return 0;
}