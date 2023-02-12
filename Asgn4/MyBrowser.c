#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>

#define INITIAL_SIZE 1024
#define PORT 80

enum RequestTypes{GET, PUT};

enum StatusCodes{
    OK=200,
    BAD_REQUEST=400,
    FORBIDDEN=403,
    NOT_FOUND=404
};

typedef struct _request_headers {
    enum RequestTypes type; //both
    char url[512]; //both
    char Host[50]; //both
    char Connection[15]; //both
    char Date[30]; //both
    char Accept[20]; //get
    char Accept_Language[20]; //get
    char If_Modified_Since[30]; //get
    char Content_Language[20]; //put
    unsigned int Content_Length; //put
    char Content_Type[20]; //put
    int isValid; //both
} RequestHeaders;

typedef struct _response_headers {
    enum RequestTypes type; //both
    enum StatusCodes statusCode;
    char Expires[25];
    char Cache_Control[15];
    char Content_Language[20];
    unsigned int Content_Length;
    char Content_Type[20];
    char Last_Modified[30];
    int isValid;
} ResponseHeaders;

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
    memset(data.ip,'\0',16);
    memset(data.route,'\0',sizeof(data.route));
    memset(scheme,'\0',8);

    strncpy(scheme, URL, 7);
    scheme[7] == '\0';
    if(strcmp(scheme, "http://") != 0) {
        printf("only http websites are supported\n");
        return data;
    }

    int startIndex = 7, endIndex = 7;
    while(URL[endIndex] != ':' && URL[endIndex] != '/' && URL[endIndex] != '\0') endIndex++;
    strncpy(data.ip, URL + startIndex, endIndex - startIndex);
    data.ip[endIndex - startIndex] = '\0';
    // no port or route provided
    if(URL[endIndex] == '\0') {
        data.port = PORT;
        return data;
    }

    // port first
    if(URL[endIndex] == ':') {
        startIndex = ++endIndex;

        while(URL[endIndex] != '/' && URL[endIndex] != '\0') endIndex++;
        char* portString = (char *)malloc(8*sizeof(char));
        memset(portString, '\0', 8);
        strncpy(portString, URL + startIndex, endIndex - startIndex);
        portString[endIndex - startIndex] = '\0';
        data.port = atoi(portString);

        // no route provided
        if(URL[endIndex] == '\0') return data;

        startIndex = ++endIndex;
        while(URL[endIndex] != '\0') endIndex++;
        strncpy(data.route, URL + startIndex, endIndex - startIndex);
        data.route[endIndex - startIndex] = '\0';
        // printf("\nData = %d %s %s\n", data.port, data.ip, data.route);
        return data;
    }

    // route first
    else if(URL[endIndex] == '/') {
        startIndex = endIndex;

        while(URL[endIndex] != ':' && URL[endIndex] != '\0') endIndex++;
        strncpy(data.route, URL + startIndex, endIndex - startIndex);
        data.route[endIndex - startIndex] = '\0';
        // no port provided
        if(URL[endIndex] == '\0') {
            data.port = PORT;
            return data;
        }

        startIndex = ++endIndex;
        while(URL[endIndex] != '\0') endIndex++;
        char* portString = (char *)malloc(8*sizeof(char));
        memset(portString, '\0', 8);
        strncpy(portString, URL + startIndex, endIndex - startIndex);
        portString[endIndex - startIndex] = '\0';
        data.port = atoi(portString);
        return data;
    }
    return data;
}

void parseResponseHeaders(char *buffer, ResponseHeaders* header) {
    int startIndex = 0, currIndex = 0;

    // http version
    while(buffer[currIndex] != ' ') currIndex++;
    char httpVersionBuf[10];
    memset(httpVersionBuf, '\0', 10);
    strncat(httpVersionBuf, buffer + startIndex, currIndex - startIndex);
    httpVersionBuf[currIndex - startIndex] = '\0';
    if(strcmp(httpVersionBuf, "HTTP/1.1") != 0) {
        header->isValid = 0;
        return; //bad response
    }

    // status code
    startIndex = ++currIndex;
    while(buffer[currIndex] != ' ') currIndex++;
    char statusCodeBuf[5];
    memset(statusCodeBuf, '\0', 5);
    strncat(statusCodeBuf, buffer + startIndex, currIndex - startIndex);
    statusCodeBuf[currIndex - startIndex] = '\0';
    header->statusCode = atoi(statusCodeBuf);

    // skip rest. goto next line
    while(buffer[currIndex] != '\r') currIndex++;
    currIndex += 2;
    startIndex = currIndex;

    int colonIndex;
    while(buffer[currIndex] != '\0') {
        if(buffer[currIndex] == ':' && buffer[currIndex+1] == ' ') colonIndex = currIndex;
        if(buffer[currIndex] == '\r') {

            if(currIndex == startIndex) break;
            if(strncmp(buffer + startIndex, "Expires", colonIndex - startIndex) == 0) {
                memset(header->Expires, '\0', 50);
                strncpy(header->Expires, buffer + colonIndex + 1, currIndex - colonIndex - 1);
                header->Expires[currIndex - colonIndex - 1] = '\0';
            }
            else if(strncmp(buffer + startIndex, "Cache-Control", colonIndex - startIndex) == 0) {
                memset(header->Cache_Control, '\0', 15);
                strncpy(header->Cache_Control, buffer + colonIndex + 1, currIndex - colonIndex - 1);
                header->Cache_Control[currIndex - colonIndex - 1] = '\0';
            }
            else if(strncmp(buffer + startIndex, "Content-Language", colonIndex - startIndex) == 0) {
                memset(header->Content_Language, '\0', 30);
                strncpy(header->Content_Language, buffer + colonIndex + 1, currIndex - colonIndex - 1);
                header->Content_Language[currIndex - colonIndex - 1] = '\0';
            }
            else if(strncmp(buffer + startIndex, "Content-Type", colonIndex - startIndex) == 0) {
                memset(header->Content_Type, '\0', 20);
                strncpy(header->Content_Type, buffer + colonIndex + 1, currIndex - colonIndex - 1);
                header->Content_Type[currIndex - colonIndex - 1] = '\0';
            }
            else if(strncmp(buffer + startIndex, "Last-Modified", colonIndex - startIndex) == 0) {
                memset(header->Last_Modified, '\0', 20);
                strncpy(header->Last_Modified, buffer + colonIndex + 1, currIndex - colonIndex - 1);
                header->Last_Modified[currIndex - colonIndex - 1] = '\0';
            }
            else if(strncmp(buffer + startIndex, "Content-Length", colonIndex - startIndex) == 0) {
                char lengthBuf[10];
                memset(lengthBuf, '\0', 10);
                strncpy(lengthBuf, buffer + colonIndex + 1, currIndex - colonIndex - 1);
                lengthBuf[currIndex - colonIndex - 1] = '\0';
                header->Content_Length = atoi(lengthBuf);
            }
            currIndex++;
            startIndex = currIndex+1;
        }
        currIndex++;
    }

    if(!(strlen(header->Expires) && strlen(header->Cache_Control) && strlen(header->Content_Language) && strlen(header->Content_Type) && strlen(header->Last_Modified)))
            header->isValid = 0;
    else
        header->isValid = 1;
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
        // send in chunks of not more than 50 bytes
        send(new_socket, buffersend, count, 0);
    }
}

char *receive_chunks(int sockfd)
{
    int n, total = 0;
    int size = INITIAL_SIZE;
    char *result = (char *)malloc(size * sizeof(char));
    for (int i = 0; i < size; i++)
        result[i] = '\0';
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
        // if (n == 0)
        // {
        //     perror("Connection closed by client");
        //     exit(1);
        // }
        total += n;
        long long l = strlen(temp);
        if (l + n > size)
        {
            size = 2 * (l + n + 1);
            result = (char *)realloc(result, size);
        }
        strcat(result, temp);
        if (temp[n - 1] == '\0')
            break;
    }
    result[total] = '\0';
    return result;
}

void write_file(int sockfd, char* mimeType){
  int n;
  FILE *fp;
  char *filename = "recv.txt";
  printf("mime: %s\n", mimeType);
  if(strcmp(mimeType, "  application/pdf") == 0 || strcmp(mimeType, "  image/jpeg") == 0) {
    printf("binary\n");
    void* buffer = (void *)malloc(52);
    fp = fopen(filename, "wb");

    while (1) {
        n = recv(sockfd, buffer, 50, 0);
        if (n <= 0){
            break;
        }
        // fprintf(fp, "%s", buffer);
        fwrite(buffer, 1, n, fp);
    }
  }
  else {
    printf("text\n");
    char* buffer = (char *)malloc(52);
    fp = fopen(filename, "w");

    while (1) {
        n = recv(sockfd, buffer, 50, 0);
        if (n <= 0){
            break;
        }
        fprintf(fp, "%s", buffer);
    }
  }

  fclose(fp);
  return;
}

char* getMimeType(char* route) {
    char* mimeType = (char *)malloc(20*sizeof(char));
    int endIndex = strlen(route);
    int startIndex = endIndex;

    while(route[startIndex] != '.' && startIndex > 0) startIndex--;
    if(startIndex == 0) return "text/*";
    startIndex++;
    strncpy(mimeType, route + startIndex, endIndex - startIndex + 1);
    mimeType[endIndex - startIndex] = '\0';

    if(strcmp(mimeType, "html") == 0) return "text/html";
    else if(strcmp(mimeType, "pdf") == 0) return "application/pdf";
    else if(strcmp(mimeType, "jpg") == 0) return "image/jpeg";
    else return "text/*";
}


int main()
{
    int connection_socket;
    struct sockaddr_in server_address; 

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
            
            // creating a socket
            if((connection_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                perror("Error in creating socket\n");
                exit(0);
            }

            server_address.sin_family = AF_INET;
            server_address.sin_port = htons(urldata.port); 
            inet_aton(urldata.ip, &server_address.sin_addr);

            // creating connection with server at specified address
            if( connect(connection_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
            {
                perror("\n Error in connecting to server \n");
                exit(0);
            }

            RequestHeaders reqHeader;
            memset(reqHeader.url,'\0', 512);
            memset(reqHeader.Host,'\0', 50);
            memset(reqHeader.Connection,'\0', 15);
            memset(reqHeader.Accept_Language,'\0', 20);
            memset(reqHeader.Accept,'\0', 20);
            strcpy(reqHeader.url, urldata.route);
            strcpy(reqHeader.Host, urldata.ip);
            strcpy(reqHeader.Connection, "close");
            strcpy(reqHeader.Accept_Language, "en-us");
            strcpy(reqHeader.Accept, getMimeType(urldata.route));

            time_t now = time(0);
            struct tm tm = *gmtime(&now);
            strftime(reqHeader.Date, sizeof(reqHeader.Date), "%a, %d %b %Y %H:%M:%S %Z", &tm);

            time_t modifyTime = now - 2*86400;
            struct tm tmModify = *gmtime(&modifyTime);
            strftime(reqHeader.If_Modified_Since, sizeof(reqHeader.If_Modified_Since), "%a, %d %b %Y %H:%M:%S %Z", &tmModify);

            char buf_data[100];
            int size = INITIAL_SIZE;
            int totalSize = 0;
            char *requestBuf = (char *)malloc(size * sizeof(char));
            for(int i = 0; i < size; i++) requestBuf[i] = '\0';

            sprintf(buf_data, "GET %s HTTP/1.1\r\n", urldata.route);
            // printf("%s\n", buf_data);
            strcat(requestBuf, buf_data);
            totalSize += strlen(buf_data);

            sprintf(buf_data, "Host: %s\r\n", reqHeader.Host);
            // printf("%s\n", buf_data);
            strcat(requestBuf, buf_data);
            totalSize += strlen(buf_data);

            sprintf(buf_data, "Connection: %s\r\n", reqHeader.Connection);
            // printf("%s\n", buf_data);
            strcat(requestBuf, buf_data);
            totalSize += strlen(buf_data);

            sprintf(buf_data, "Date: %s\r\n", reqHeader.Date);
            // printf("%s\n", buf_data);
            strcat(requestBuf, buf_data);
            totalSize += strlen(buf_data);

            sprintf(buf_data, "Accept: %s\r\n", reqHeader.Accept);
            // printf("%s\n", buf_data);
            strcat(requestBuf, buf_data);
            totalSize += strlen(buf_data);

            sprintf(buf_data, "Accept-Language: %s\r\n", reqHeader.Accept_Language);
            // printf("%s\n", buf_data);
            strcat(requestBuf, buf_data);
            totalSize += strlen(buf_data);

            sprintf(buf_data, "If-Modified-Since: %s\r\n", reqHeader.If_Modified_Since);
            // printf("%s\n", buf_data);
            strcat(requestBuf, buf_data);
            totalSize += strlen(buf_data);

            sprintf(buf_data, "\r\n");
            // printf("%s\n", buf_data);
            strcat(requestBuf, buf_data);
            totalSize += strlen(buf_data);
            requestBuf[totalSize] = '\0';

            send_chunks(connection_socket, requestBuf);

            char *responseResult;
            responseResult = receive_chunks(connection_socket);
            
            // parse the response
            ResponseHeaders resHeaders;
            parseResponseHeaders(responseResult, &resHeaders);

            if(resHeaders.statusCode == OK) {
                // read file and save to disk and open using an app.
                write_file(connection_socket, resHeaders.Content_Type);
            }
            else {
                // if content-type is text/html, show it in browser. if text/* print in terminalk. else ignore.
                printf("kuch toh error hai\n");
            }

            close(connection_socket);
        }
        else if (strcmp(cmd[0], "PUT") == 0)
        {
            // do nothin;
            printf("\nip %s fname %s", cmd[1], cmd[2]);
        }
    }
    return 0;
}