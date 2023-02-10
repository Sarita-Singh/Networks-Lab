#include <stdio.h>
#include <string.h>
#include <stdlib.h>

const char *code_200 = "200";
const char *code_400 = "400";
const char *code_403 = "403";
const char *code_404 = "404";

// Function to parse the command and break it into appropriate tokens
char **parseCommand(char *command, int cnt)
{
    char **cmd = (char **)malloc(cnt * sizeof(char *));
    int i = 0;
    cnt = 0;
    char buf[100];
    while (i < strlen(command))
    {
        memset(buf, '\0', sizeof(buf));
        int j = 0;
        while (i < strlen(command) && command[i] != ' ')
        {
            buf[j++] = command[i++];
        }
        cmd[cnt] = (char *)malloc((strlen(buf) + 1) * sizeof(char));
        strcpy(cmd[cnt++], buf);
        while (i < strlen(command) && command[i] == ' ')
        {
            i++;
        }
    }
    return cmd;
}

int main()
{
    char ch;
    char *inp_cmd = (char *)malloc(200 * sizeof(char));
    int status = 0, args = 0;
    while (1)
    {
        printf("\nMyOwnBrowser> ");

        long long c = 0;
        // loop to read command untill newline is entered
        do
        {
            ch = getchar();
            inp_cmd[c] = ch;
            c++;
        } while (ch != '\n');
        c = c - 1;
        inp_cmd[c] = '\0';
        char **cmd = parseCommand(inp_cmd, &args);
        if (!cmd[0])
        {
            continue;
        }
        if (strcmp(cmd[0], "QUIT") == 0)
            break;
        else if (strcmp(cmd[0], "GET") == 0)
        {
            // do nothin;
            printf("\nip %s", cmd[1]);
        }
        else if (strcmp(cmd[0], "PUT") == 0)
        {
            // do nothin;
            printf("\nip %s fname %s", cmd[1], cmd[2]);
        }
    }
    return 0;
}