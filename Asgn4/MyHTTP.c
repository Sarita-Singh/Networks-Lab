#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/sendfile.h>
#include <errno.h>
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
    enum StatusCodes statusCode;
    char Expires[30];
    char Cache_Control[15];
    char Content_Language[20];
    unsigned int Content_Length;
    char Content_Type[20];
    char Last_Modified[30];
    int isValid;
} ResponseHeaders;

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

void send_chunks(int new_socket, char *result)
{
    char buffersend[52];
    int count = 0;
    int res_len = strlen(result)+1;
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

// send file in chunks
void send_file(int sockfd, FILE *fp, char* mimeType){
  int total = 0;

  if(strcmp(mimeType, "application/pdf") == 0 || strcmp(mimeType, "image/jpeg") == 0) {
    void* data = (void *)malloc(52);
    bzero(data, 52);
    while(fread(data, 1, 50, fp) != 0) {
        total += send(sockfd, data, 50, 0);
        bzero(data, 52);
    }
  }
  else {
    char data[52];
    bzero(data, 52);
    while(fgets(data, 50, fp) != NULL) {
        total += send(sockfd, data, 50, 0);
        bzero(data, 52);
    }
  }
}

void parseRequestHeaders(char *buffer, RequestHeaders* header) {
    int startIndex = 0, currIndex = 0;

    // request type
    while(buffer[currIndex] != ' ') currIndex++;
    char requestTypeBuf[5];
    memset(requestTypeBuf, '\0', 5);
    strncat(requestTypeBuf, buffer + startIndex, currIndex - startIndex);
    requestTypeBuf[currIndex - startIndex] = '\0';
    if(strcmp(requestTypeBuf, "GET") == 0) header->type = GET;
    else if(strcmp(requestTypeBuf, "PUT") == 0) header->type = PUT;
    else {
        header->isValid = 0;
        return; //bad request
    }

    // url
    startIndex = ++currIndex;
    while(buffer[currIndex] != ' ') currIndex++;
    memset(header->url, '\0', 512);
    strncat(header->url, buffer + startIndex, currIndex - startIndex);
    header->url[currIndex - startIndex] = '\0';

    //http version
    startIndex = currIndex + 1;
    char httpVersionBuf[10];
    while(buffer[currIndex] != '\r') currIndex++;
    memset(httpVersionBuf, '\0', 10);
    strncat(httpVersionBuf, buffer + startIndex, currIndex - startIndex);
    httpVersionBuf[currIndex - startIndex] = '\0';
    if(strcmp(httpVersionBuf, "HTTP/1.1") != 0) {
        header->isValid = 0;
        return; //bad request
    }

    // start of request headers
    // skip \n
    currIndex += 2;
    startIndex = currIndex;
    int colonIndex;
    while(currIndex < strlen(buffer) && (!(buffer[currIndex] == '\n' && buffer[currIndex+1] == '\r' && buffer[currIndex+2] == '\n'))) {
        if(buffer[currIndex] == ':' && buffer[currIndex+1] == ' ') colonIndex = currIndex;
        if(buffer[currIndex] == '\r') {

            if(currIndex == startIndex) break;
            if(strncmp(buffer + startIndex, "Host", colonIndex - startIndex) == 0) {
                memset(header->Host, '\0', 50);
                strncpy(header->Host, buffer + colonIndex + 2, currIndex - colonIndex - 2);
                header->Host[currIndex - colonIndex - 2] = '\0';
            }
            else if(strncmp(buffer + startIndex, "Connection", colonIndex - startIndex) == 0) {
                memset(header->Connection, '\0', 15);
                strncpy(header->Connection, buffer + colonIndex + 2, currIndex - colonIndex - 2);
                header->Connection[currIndex - colonIndex - 2] = '\0';
            }
            else if(strncmp(buffer + startIndex, "Date", colonIndex - startIndex) == 0) {
                memset(header->Date, '\0', 30);
                strncpy(header->Date, buffer + colonIndex + 2, currIndex - colonIndex - 2);
                header->Date[currIndex - colonIndex - 2] = '\0';
            }
            else if((!strcmp(requestTypeBuf, "GET")) && (strncmp(buffer + startIndex, "Accept", colonIndex - startIndex) == 0)) {
                memset(header->Accept, '\0', 20);
                strncpy(header->Accept, buffer + colonIndex + 2, currIndex - colonIndex - 2);
                header->Accept[currIndex - colonIndex - 2] = '\0';
            }
            else if((!strcmp(requestTypeBuf, "GET")) && (strncmp(buffer + startIndex, "Accept-Language", colonIndex - startIndex) == 0)) {
                memset(header->Accept_Language, '\0', 20);
                strncpy(header->Accept_Language, buffer + colonIndex + 2, currIndex - colonIndex - 2);
                header->Accept_Language[currIndex - colonIndex - 2] = '\0';
            }
            else if((!strcmp(requestTypeBuf, "GET")) && (strncmp(buffer + startIndex, "If-Modified-Since", colonIndex - startIndex) == 0)) {
                memset(header->If_Modified_Since, '\0', 30);
                strncpy(header->If_Modified_Since, buffer + colonIndex + 2, currIndex - colonIndex - 2);
                header->If_Modified_Since[currIndex - colonIndex - 2] = '\0';
            }
            else if((!strcmp(requestTypeBuf, "PUT")) && (strncmp(buffer + startIndex, "Content-Language", colonIndex - startIndex) == 0)) {
                memset(header->Content_Language, '\0', 20);
                strncpy(header->Content_Language, buffer + colonIndex + 2, currIndex - colonIndex - 2);
                header->Content_Language[currIndex - colonIndex - 2] = '\0';
            }
            else if((!strcmp(requestTypeBuf, "PUT")) && (strncmp(buffer + startIndex, "Content-Type", colonIndex - startIndex) == 0)) {
                memset(header->Content_Type, '\0', 20);
                strncpy(header->Content_Type, buffer + colonIndex + 2, currIndex - colonIndex - 2);
                header->Content_Type[currIndex - colonIndex - 2] = '\0';
            }
            else if((!strcmp(requestTypeBuf, "PUT")) && (strncmp(buffer + startIndex, "Content-Length", colonIndex - startIndex) == 0)) {
                char lengthBuf[20];
                memset(lengthBuf, '\0', 20);
                strncpy(lengthBuf, buffer + colonIndex + 2, currIndex - colonIndex - 2);
                lengthBuf[currIndex - colonIndex - 2] = '\0';
                int x = atoi(lengthBuf);
                header->Content_Length = x;
            }
            currIndex++;
            startIndex = currIndex+1;
        }
        currIndex++;
    }
    currIndex += 3;
    buffer += currIndex;
    if(header->type == GET) {
        if(!(strlen(header->Host) && strlen(header->Connection) && strlen(header->Date) && strlen(header->Accept) && strlen(header->Accept_Language) && strlen(header->If_Modified_Since)))
            header->isValid = 0;
        else
            header->isValid = 1;
    }

    if(header->type == PUT) {
        if(!(strlen(header->Host) && strlen(header->Connection) && strlen(header->Date) && strlen(header->Content_Language) && strlen(header->Content_Type)))
            header->isValid = 0;
        else
            header->isValid = 1;
    }
}

void write_file(int sockfd, char* mimeType, char *filename, unsigned int size){
  int n=0, total = 0;
  FILE *fp;
  
  if(strcmp(mimeType, "application/pdf") == 0 || strcmp(mimeType, "image/jpeg") == 0) {
    void* buffer = (void *)malloc(52);
    fp = fopen(filename, "wb+");
    if (fp == NULL) {
        perror("Error in reading file.");
        return;
    }
    while (1) {
        bzero(buffer, 52);
        n = recv(sockfd, buffer, 50, 0);
        total += n;
        fwrite(buffer, 1, n, fp);
        if (n <= 0 || total >= size){
            break;
        }
    }
  }
  else {
    char* buffer = (char *)malloc(52);
    fp = fopen(filename, "w+");
    if (fp == NULL) {
        perror("Error in reading file.");
        return;
    }
    while (1) {
        memset(buffer, '\0', 52);
        n = recv(sockfd, buffer, 50, 0);
        total += n;
        fprintf(fp, "%s", buffer);
        if(buffer[n-1] == '\0') break;
        if (n <= 0 || total >= size){
            break;
        }
    }
  }
  fclose(fp);
  return;
}


int main()
{
    int server_socket;
    int clilen, error_check;

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("\nError in creating socket\n");
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

    int i, newsockfd;

    while (1)
    {
        clilen = sizeof(client_address);

        // client socket stored in variable newsockfd after accepting the connection
        if ((newsockfd = accept(server_socket, (struct sockaddr *)&client_address, &clilen)) < 0)
        {
            perror("\nError in accept\n");
            continue;
        }

        char *clientIP = inet_ntoa(client_address.sin_addr);
        int clientPORT = ntohs(client_address.sin_port);
        
        char *result;
        int cnt = 0;

        result = receive_chunks(newsockfd);
        printf("\n%s\n", result);
        
        // parse the headerline to get data
        RequestHeaders reqHeaders;
        parseRequestHeaders(result, &reqHeaders);

        // write to AccessLog.txt
        FILE *filePointer;
        filePointer = fopen("AccessLog.txt", "aw");
        time_t t;
        t = time(NULL);
        struct tm tm = *localtime(&t);
        fprintf(filePointer, "%d-%d-%d:%d:%d:%d:%s:%d:%s:%s\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec, clientIP, clientPORT, reqHeaders.type == GET ? "GET" : "PUT", reqHeaders.url);
        fclose(filePointer);

        // Prepare response headers
        ResponseHeaders resHeaders;
        memset(resHeaders.Cache_Control, '\0', 15);
        memset(resHeaders.Content_Language, '\0', 20);
        memset(resHeaders.Expires, '\0', 30);
        memset(resHeaders.Last_Modified, '\0', 30);
        memset(resHeaders.Content_Type, '\0', 20);
        strcpy(resHeaders.Cache_Control, "no-store");
        strcpy(resHeaders.Content_Language, "en-us");

        time_t ExpireTime = time(0) + 3*86400;
        struct tm tmExpire = *gmtime(&ExpireTime);
        strftime(resHeaders.Expires, sizeof(resHeaders.Expires), "%a, %d %b %Y %H:%M:%S %Z", &tmExpire);
        //get filename
        char filename[512];
        memset(filename, '\0', 512);
        if(reqHeaders.type == PUT) strcat(filename, "server-"); // for debugging
        strcat(filename, reqHeaders.url+1);
        size_t filesize;

        // receive and write to file for PUT
        if(reqHeaders.type == PUT) {
            write_file(newsockfd, reqHeaders.Content_Type, filename, reqHeaders.Content_Length);
        }

        // get filesize incase of GET
        if(reqHeaders.type == GET) {
            struct stat s = {0};

            if (!(stat(filename, &s)))
            {
                if (ENOENT == errno)
                // file does not exist
                resHeaders.statusCode = NOT_FOUND;
            }
            else {
                filesize = s.st_size;

                struct tm tmModify = *gmtime(&(s.st_mtim));
                strftime(resHeaders.Last_Modified, sizeof(resHeaders.Last_Modified), "%a, %d %b %Y %H:%M:%S %Z", &tmModify);
            }
        }

        char buf_data[100];
        int size = INITIAL_SIZE;
        int totalSize = 0;
        char *responseBuf = (char *)malloc(size * sizeof(char));
        for(int i = 0; i < size; i++) responseBuf[i] = '\0';


        if(!reqHeaders.isValid) {
            // Send 400
            resHeaders.statusCode = BAD_REQUEST;

            char content[100];
            sprintf(content,"Bad Request\r\n");
            resHeaders.Content_Length = strlen(content);
            strcpy(resHeaders.Content_Type, "text/*");

            sprintf(buf_data, "HTTP/1.1 %d %s\r\n", resHeaders.statusCode, "BAD REQUEST");
            strcat(responseBuf, buf_data);
            totalSize += strlen(buf_data); 

            sprintf(buf_data, "Expires: %s\r\n", resHeaders.Expires);
            strcat(responseBuf, buf_data);
            totalSize += strlen(buf_data); 

            sprintf(buf_data, "Cache-Control: %s\r\n", resHeaders.Cache_Control);
            strcat(responseBuf, buf_data);
            totalSize += strlen(buf_data); 

            sprintf(buf_data, "Content-Language: %s\r\n", resHeaders.Content_Language);
            strcat(responseBuf, buf_data);
            totalSize += strlen(buf_data); 

            sprintf(buf_data, "Content-Length: %d\r\n", resHeaders.Content_Length);
            strcat(responseBuf, buf_data);
            totalSize += strlen(buf_data); 

            sprintf(buf_data, "Content-Type: %s\r\n", resHeaders.Content_Type);
            strcat(responseBuf, buf_data);
            totalSize += strlen(buf_data); 

            sprintf(buf_data, "Last-Modified: %s\r\n", resHeaders.Last_Modified);
            strcat(responseBuf, buf_data);
            totalSize += strlen(buf_data); 

            sprintf(buf_data, "\r\n");
            strcat(responseBuf, buf_data);
            totalSize += strlen(buf_data); 

            strcat(responseBuf, content);
            totalSize += strlen(content);             
            responseBuf[totalSize] = '\0';
            send_chunks(newsockfd, responseBuf);
        }
        else {
            // Send 200
            resHeaders.statusCode = OK;

            char content[100]; // for PUT

            if(reqHeaders.type == GET) {
                memset(resHeaders.Content_Type, '\0', 20);
                strcpy(resHeaders.Content_Type, reqHeaders.Accept);
                resHeaders.Content_Length = (unsigned int)filesize;
            }
            else {
                sprintf(content,"Success\r\n");
                resHeaders.Content_Length = strlen(content);
                strcpy(resHeaders.Content_Type, "text/*");
            }

            sprintf(buf_data, "HTTP/1.1 %d %s\r\n", resHeaders.statusCode, "OK");
            strcat(responseBuf, buf_data);
            totalSize += strlen(buf_data); 

            sprintf(buf_data, "Expires: %s\r\n", resHeaders.Expires);
            strcat(responseBuf, buf_data);
            totalSize += strlen(buf_data); 

            sprintf(buf_data, "Cache-Control: %s\r\n", resHeaders.Cache_Control);
            strcat(responseBuf, buf_data);
            totalSize += strlen(buf_data); 

            sprintf(buf_data, "Content-Language: %s\r\n", resHeaders.Content_Language);
            strcat(responseBuf, buf_data);
            totalSize += strlen(buf_data); 

            sprintf(buf_data, "Content-Length: %d\r\n", resHeaders.Content_Length);
            strcat(responseBuf, buf_data);
            totalSize += strlen(buf_data); 

            sprintf(buf_data, "Content-Type: %s\r\n", resHeaders.Content_Type);
            strcat(responseBuf, buf_data);
            totalSize += strlen(buf_data); 

            sprintf(buf_data, "Last-Modified: %s\r\n", resHeaders.Last_Modified);
            strcat(responseBuf, buf_data);
            totalSize += strlen(buf_data); 

            sprintf(buf_data, "\r\n");
            strcat(responseBuf, buf_data);
            totalSize += strlen(buf_data); 
            responseBuf[totalSize] = '\0';

            if(reqHeaders.type == PUT) {
                strcat(responseBuf, content);
                totalSize += strlen(content);             
                responseBuf[totalSize] = '\0';
            }

            send_chunks(newsockfd, responseBuf);

            if(reqHeaders.type == GET) {
                //open file in appropriate format
                FILE *fp;
                fp = strcmp(reqHeaders.Accept, "application/pdf") == 0 || strcmp(reqHeaders.Accept, "image/jpeg") == 0 ? fopen(filename, "rb") : fopen(filename, "r");
                if (fp == NULL) {
                    perror("Error in reading file.");
                }

                //now send file
                send_file(newsockfd, fp, reqHeaders.Accept);
            }
        }

        close(newsockfd);
    }

    return 0;
}