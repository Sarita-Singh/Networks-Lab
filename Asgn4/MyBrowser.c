#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct _url_data {
    int port;
    char* ip;
    char* route;
} URLData;

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

// Function to parse the url and extract ip address, port and route from it
URLData parseURL(char* URL) {
    URLData data;
    data.ip = (char *)malloc(16*sizeof(char)); // 255.255.255.255 will take 16 bytes
    data.route = (char *)malloc(strlen(URL)*sizeof(char)); // length cannot be more than total URL length
    char* scheme = (char *)malloc(8*sizeof(char)); // "http://" will take 8 bytes
    
    strncpy(scheme, URL, 7);
    scheme[7] == '\0';
    if(strcmp(scheme, "http://") != 0) {
        printf("only http websites are supported\n");
        return data;
    }

    int startIndex = 7, endIndex = 7;
    while(URL[endIndex] != ':' && URL[endIndex] != '/' && URL[endIndex] != '\0') endIndex++;
    strncpy(data.ip, URL + startIndex, endIndex - startIndex);

    // no port or route provided
    if(URL[endIndex] == '\0') {
        data.port = 80;
        return data;
    }

    // port first
    if(URL[endIndex] == ':') {
        startIndex = ++endIndex;

        while(URL[endIndex] != '/' && URL[endIndex] != '\0') endIndex++;
        char* portString = (char *)malloc(8*sizeof(char));
        strncpy(portString, URL + startIndex, endIndex - startIndex);
        data.port = atoi(portString);

        // no route provided
        if(URL[endIndex] == '\0') return data;

        startIndex = endIndex;
        while(URL[endIndex] != '\0') endIndex++;
        strncpy(data.route, URL + startIndex, endIndex - startIndex);
        return data;
    }

    // route first
    else if(URL[endIndex] == '/') {
        startIndex = endIndex;

        while(URL[endIndex] != ':' && URL[endIndex] != '\0') endIndex++;
        strncpy(data.route, URL + startIndex, endIndex - startIndex);

        // no port provided
        if(URL[endIndex] == '\0') {
            data.port = 80;
            return data;
        }

        startIndex = ++endIndex;
        while(URL[endIndex] != '\0') endIndex++;
        char* portString = (char *)malloc(8*sizeof(char));
        strncpy(portString, URL + startIndex, endIndex - startIndex);
        data.port = atoi(portString);
        return data;
    }

    return data;
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

        char **cmd = parseCommand(inp_cmd, args);

        if (!cmd[0])
        {
            continue;
        }
        if (strcmp(cmd[0], "QUIT") == 0)
            break;
        else if (strcmp(cmd[0], "GET") == 0)
        {
            URLData urldata = parseURL(cmd[1]);
            printf("ip: %s\n", urldata.ip);
            printf("port: %d\n", urldata.port);
            printf("route: %s\n", urldata.route);
        }
        else if (strcmp(cmd[0], "PUT") == 0)
        {
            // do nothin;
            printf("\nip %s fname %s", cmd[1], cmd[2]);
        }
    }
    return 0;
}