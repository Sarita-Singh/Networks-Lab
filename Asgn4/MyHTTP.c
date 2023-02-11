#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#define INITIAL_SIZE 512
#define PORT 80

enum RequestTypes{GET, PUT};

enum StatusCodes{
    OK=200,
    BAD_REQUEST=400,
    FORBIDDEN=403,
    NOT_FOUND=404
};

typedef struct _request_headers {
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
    enum StatusCodes statusCode;
    char Expires[25];
    char Cache_Control[15];
    char Content_Language[20];
    unsigned int Content_Length;
    char Content_Type[20];
    char Last_Modified[30];
} ResponseHeaders;

char *receive_chunks(int sockfd)
{
    int n, total = 0;
    int size = INITIAL_SIZE;
    char *result = (char *)malloc(size * sizeof(char));
    for(int i = 0; i < size; i++) result[i] = '\0';
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
        if (n == 0)
        {
            perror("Connection closed by client");
            exit(1);
        }
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

void parseRequestHeaders(char *buffer, RequestHeaders* header) {
    int startIndex = 0, currIndex = 0;
    enum RequestTypes requestType;

    // request type
    while(buffer[currIndex] != ' ') currIndex++;
    char requestTypeBuf[5];
    strncat(requestTypeBuf, buffer + startIndex, currIndex - startIndex);
    if(strcmp(requestTypeBuf, "GET") == 0) requestType = GET;
    else if(strcmp(requestTypeBuf, "PUT") == 0) requestType = PUT;
    else {
        header->isValid = 0;
        return; //bad request
    }

    // url
    startIndex = currIndex + 1;
    while(buffer[currIndex] != ' ') currIndex++;
    strncat(header->url, buffer + startIndex, currIndex - startIndex);

    //http version
    startIndex = currIndex + 1;
    char httpVersionBuf[5];
    while(buffer[currIndex] != '\r') currIndex++;
    strncat(httpVersionBuf, buffer + startIndex, currIndex - startIndex);
    if(strcmp(httpVersionBuf, "HTTP/1.1") != 0) {
        header->isValid = 0;
        return; //bad request
    }

    // start of request headers
    // skip \n
    startIndex = currIndex + 2;

    int colonIndex;
    while(buffer[currIndex] != '\0') {
        if(buffer[currIndex] == ':') colonIndex = currIndex;
        if(buffer[currIndex] == '\r') {
            if(strncmp(buffer + startIndex, "Host", colonIndex - startIndex) == 0) {
                strncpy(header->Host, buffer + colonIndex + 1, currIndex - colonIndex - 1);
                header->Host[currIndex - colonIndex - 1] = '\0';
            }
            else if(strncmp(buffer + startIndex, "Connection", colonIndex - startIndex) == 0) {
                strncpy(header->Connection, buffer + colonIndex + 1, currIndex - colonIndex - 1);
                header->Connection[currIndex - colonIndex - 1] = '\0';
            }
            else if(strncmp(buffer + startIndex, "Date", colonIndex - startIndex) == 0) {
                strncpy(header->Date, buffer + colonIndex + 1, currIndex - colonIndex - 1);
                header->Date[currIndex - colonIndex - 1] = '\0';
            }
            else if(strncmp(buffer + startIndex, "Accept", colonIndex - startIndex) == 0) {
                strncpy(header->Accept, buffer + colonIndex + 1, currIndex - colonIndex - 1);
                header->Accept[currIndex - colonIndex - 1] = '\0';
            }
            else if(strncmp(buffer + startIndex, "Accept-Language", colonIndex - startIndex) == 0) {
                strncpy(header->Accept_Language, buffer + colonIndex + 1, currIndex - colonIndex - 1);
                header->Accept_Language[currIndex - colonIndex - 1] = '\0';
            }
            else if(strncmp(buffer + startIndex, "If-Modified-Since", colonIndex - startIndex) == 0) {
                strncpy(header->If_Modified_Since, buffer + colonIndex + 1, currIndex - colonIndex - 1);
                header->If_Modified_Since[currIndex - colonIndex - 1] = '\0';
            }
            else if(strncmp(buffer + startIndex, "Content-Language", colonIndex - startIndex) == 0) {
                strncpy(header->Content_Language, buffer + colonIndex + 1, currIndex - colonIndex - 1);
                header->Content_Language[currIndex - colonIndex - 1] = '\0';
            }
            else if(strncmp(buffer + startIndex, "Content-Type", colonIndex - startIndex) == 0) {
                strncpy(header->Content_Type, buffer + colonIndex + 1, currIndex - colonIndex - 1);
                header->Content_Type[currIndex - colonIndex - 1] = '\0';
            }
            else if(strncmp(buffer + startIndex, "Content-Length", colonIndex - startIndex) == 0) {
                char lengthBuf[20];
                strncpy(lengthBuf, buffer + colonIndex + 1, currIndex - colonIndex - 1);
                lengthBuf[currIndex - colonIndex - 1] = '\0';
                header->Content_Length = atoi(lengthBuf);
            }
            currIndex++;
        }
        currIndex++;
    }

    if(requestType == GET) {
        if(strlen(header->Host) && strlen(header->Connection) && strlen(header->Date) && strlen(header->Accept) && strlen(header->Accept_Language) && strlen(header->If_Modified_Since))
            header->isValid = 0;
        else
            header->isValid = 1;
    }

    if(requestType == PUT) {
        if(strlen(header->Host) && strlen(header->Connection) && strlen(header->Date) && strlen(header->Content_Language) && strlen(header->Content_Length) && strlen(header->Content_Type))
            header->isValid = 0;
        else
            header->isValid = 1;
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
    printf("\nSocket created\n");
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
    printf("\nBind done\n");
    // Up to 5 concurrent client requests will be queued up while the system is
    // executing the accept system call
    error_check = listen(server_socket, 5);
    if (error_check == -1)
    {
        perror("\nError in listening\n");
        exit(0);
    }
    printf("\nServer listening at port:%d\n", PORT);
    char buf[50]; // buffer used for communication
    char username[50];
    memset(&buf, '\0', sizeof(buf));
    memset(&username, '\0', sizeof(username));

    int i, newsockfd;
    
    // open the file AccessLog.txt
    FILE *filePointer ; 
    
    while (1)
    {
        clilen = sizeof(client_address);

        // client socket stored in variable newsockfd after accepting the connection
        if ((newsockfd = accept(server_socket, (struct sockaddr *)&client_address, &clilen)) < 0)
        {
            perror("\nError in accept\n");
            exit(0);
        }
        filePointer = fopen("AccessLog.txt", "aw");
        char *clientIP = inet_ntoa(client_address.sin_addr);
        int clientPORT = ntohs(client_address.sin_port);
        // printf("%s:%d\n", clientIP, clientPORT);
        printf("\nClient connected\n");
        char *result;
        int cnt=0;
        char headerLine[INITIAL_SIZE];

        RequestHeaders reqHeaders;

        // first receive in a while loop till empty line
        while(1){
            // keep receiving each line of header
            result = receive_chunks(newsockfd);
            if(strcmp(result, "\r\n") == 0){
                // header over
                break;
            }
            memset(&headerLine, '\0', sizeof(headerLine));
            strcpy(headerLine, result);
            // parse the headerline to get data
            if(!cnt){
                // get time, client ip, port
                time_t t;
                t = time(NULL);
                struct tm tm = *localtime(&t);
                fprintf(filePointer, "%d-%d-%d:%d:%d:%d:%s:%d\n", tm.tm_mday, tm.tm_mon+1, tm.tm_year+1900, tm.tm_hour, tm.tm_min, tm.tm_sec, clientIP, clientPORT);                
                cnt=1;
            }
            printf("\n%s", headerLine);

        }
        // then receive the body if command was 'PUT'
        fclose(filePointer);

        close(newsockfd);
    }
    
    return 0;
}