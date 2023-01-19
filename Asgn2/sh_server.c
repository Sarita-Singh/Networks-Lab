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

int check_usrname(char *username)
{
    FILE *fp = fopen("user_name.txt", "r");
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

int receive(int sockfd, char *buf, int size, char delim)
{
    char temp[50];
    int total = 0;
    while (1)
    {
        memset(temp, '\0', sizeof(temp));
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

// Function to handle the 'dir' command
int _dir(char *sh_cmd, char *result)
{
    char *temp_buf = (char *)malloc(1024 * sizeof(char));
    long long cmd_len = strlen(sh_cmd);
    char currrent_dir[1024];
    long long idx = 3, temp_len = 0;
    while (idx < cmd_len && sh_cmd[idx] == ' ')
    {
        idx++;
    }

    if (idx < cmd_len)
    {
        strcat(currrent_dir, sh_cmd + idx);
        if (sh_cmd[cmd_len - 1] != '/')
            strcat(currrent_dir, "/");
    }
    else
    {
        getcwd(currrent_dir, sizeof(currrent_dir));
    }
    DIR *dir = opendir(currrent_dir);
    if (dir)
    {
        struct dirent *entry_dir;
        char *temp_buf = (char *)malloc(1024 * sizeof(char));

        while ((entry_dir = readdir(dir)) != NULL)
        {
            long long len = strlen(entry_dir->d_name);
            long long lres = strlen(temp_buf);
            if (len + lres + 1 >= temp_len)
            {
                temp_len = len + lres + 2;
                temp_buf = (char *)realloc(temp_buf, temp_len);
            }
            strcat(temp_buf, entry_dir->d_name);
            strcat(temp_buf, "\n");
        }
        long long lres = strlen(result);
        if (lres > 0)
        {
            temp_buf[lres - 1] = '\0';
        }
        strcpy(result, temp_buf);
        free(temp_buf);
    }
    else
    {
        perror("\nDirectory could not be opened\n");
        return 0;
    }

    closedir(dir);
    return 1;
}

int _cd(char *sh_cmd)
{
    long long cmd_len = strlen(sh_cmd);
    char changedir[1024] = "\0";
    long long idx = 2;
    while (idx < cmd_len && sh_cmd[idx] == ' ')
    {
        idx++;
    }
    if (idx < cmd_len)
    {
        strcat(changedir, sh_cmd + idx);
        if (sh_cmd[cmd_len - 1] != '/')
            strcat(changedir, "/");
    }
    else
    {
        char *temp = getenv("HOME");
        strcpy(changedir, temp);
    }
    if (chdir(changedir) < 0)
    {
        return 0;
    }
    return 1;
}

int _pwd(char *sh_cmd, char *result)
{
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL)
        return 0;
    strcpy(result, cwd);
    result[strlen(result)] = '\0';
    printf("Current working dir: %s\n", result);

    return 1;
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
    {    perror("\nError in listening\n");
        exit(0);}

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

            int cnt = 0;

            strcpy(buf, "LOGIN:");
            buf[strlen(buf)]='\0';
            // sending the string "LOGIN:"
            int n = send(new_socket, buf, strlen(buf) + 1, 0);
            if (n == 0)
            {
                perror("\nClient disconnected.\n");
                break;
            }
            memset(&username, '\0', 50);
            receive(new_socket, username, 50, '\0');

            printf("Username = %s\n", username);
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
                char sh_cmd[COMMAND_SIZE];
                memset(&sh_cmd, '\0', COMMAND_SIZE);

                // receive the shell command
                receive(new_socket, sh_cmd, COMMAND_SIZE, '\0');

                printf("%s", sh_cmd);
                if (strcmp(sh_cmd, "exit") == 0)
                {
                    close(new_socket);
                    exit(0);
                }
                // execute the shell command
                char result[COMMAND_SIZE];
                memset(&result, '\0', COMMAND_SIZE);
                
                char cmdrun_err[5], invalid_cmd[5];
                strcpy(cmdrun_err, "####");
                // cmdrun_err[4] = '\0';
                strcpy(invalid_cmd, "$$$$");
                // invalid_cmd[4] = '\0';

                if (strcmp(sh_cmd, "pwd") == 0)
                {
                    int ret = _pwd(sh_cmd, result);
                    if (ret == 0)
                    {   
                        strcpy(result, cmdrun_err);
                        // send(new_socket, cmdrun_err, strlen(cmdrun_err) + 1, 0);
                        // continue;
                    }
                }
                else if (strcmp(sh_cmd, "dir") == 0)
                {
                    int ret = _dir(sh_cmd, result);
                    if (ret == 0)
                    {   
                        strcpy(result, cmdrun_err);
                        // send(new_socket, cmdrun_err, strlen(cmdrun_err) + 1, 0);
                        // continue;
                    }
                }
                else if (sh_cmd[0] == 'c' && sh_cmd[1] == 'd')
                {
                    int ret = _cd(sh_cmd);
                    if (ret == 0)
                    {   
                        strcpy(result, cmdrun_err);
                        // send(new_socket, cmdrun_err, sizeof(cmdrun_err), 0);
                        // continue;
                    }
                }
                else {
                    send(new_socket, invalid_cmd, sizeof(invalid_cmd), 0);
                    continue;
                }

                char res_buf[50];
                int l = strlen(result);
				result[l] = '\0';
				int szbuf = 50;
                l++;
				printf("\n%s\n",result);
				for (int i = 0; i < l; i += szbuf)
				{
					memset(&res_buf, '\0', 50);
					int cnt = 0;
					for (int j = 0; j < szbuf && i + j < l; j++)
					{
						cnt++;
						res_buf[j] = result[i + j];
					}
					send(new_socket, res_buf, cnt, 0);
				}
            }

            close(new_socket);
            exit(0);
        }
        close(new_socket);
    }
    return 0;
}