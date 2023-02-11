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

enum StatusCodes{
OK=200,
BAD_REQUEST=400,
FORBIDDEN=403,
NOT_FOUND=404
};

typedef struct _get_request_headers {
    char Host[50];
    char Connection[15];
    char Date[30];
    char Accept[100];
    char Accept_Language[20];
    char If_Modified_Since[30];
}GetRequestHeaders;

typedef struct _put_request_headers {
    char Host[50];
    char Connection[15];
    char Date[30];
    char Content_Language[20];
    unsigned int Content_Length;
    char Content_Type[100];
}PutRequestHeaders;

typedef struct _get_response_headers {
    enum StatusCodes statusCode;
    char Expires[25];
    char Cache_Control[15];
    char Content_Language[20];
    unsigned int Content_Length;
    char Content_Type[100];
    char Last_Modified[30];
}GetResponseHeaders;

typedef struct _put_response_headers {
    enum StatusCodes statusCode;
    char Expires[25];
    char Cache_Control[15];
    char Content_Language[20];
    unsigned int Content_Length;
    char Content_Type[100];
    char Last_Modified[30];
}PutResponseHeaders;

char *receive_chunks(int sockfd)
{
    int n, total = 0;
    int size = INITIAL_SIZE;
    char *result = (char *)malloc(size * sizeof(char));
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
    return result;
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
    filePointer = fopen("AccessLog.txt", "w");
    while (1)
    {
        clilen = sizeof(client_address);

        // client socket stored in variable newsockfd after accepting the connection
        if ((newsockfd = accept(server_socket, (struct sockaddr *)&client_address, &clilen)) < 0)
        {
            perror("\nError in accept\n");
            exit(0);
        }

        // buffer to store the date and time
        char buf_date_time[50];
        time_t curr_time;
        
        printf("\nClient connected\n");
        char *result;
        int cnt=0;
        char *headerLine;

        // first receive in a while loop till empty line
        while(1){
            if(strcmp(result, "\r\n") == 0){
                // header over
                break;
            }
            // keep receiving each line of header
            result = receive_chunks(newsockfd);
            strcpy(headerLine, result);
            // parse the headerline to get data
            if(!cnt){
                // get time, client ip, port
                curr_time = time(NULL);
                // ctime used to get string representing local time for the curr_time
                snprintf(buf_date_time, sizeof(buf_date_time), "\a%s\n", ctime(&curr_time));

                fprintf(filePointer, "%s", "We", buf_date_time);
                cnt=1;
            }

        }
        // then receive the body if command was 'PUT'
        printf("\n%s", result);

        
        close(newsockfd);
    }
    return 0;
}