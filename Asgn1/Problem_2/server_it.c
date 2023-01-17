/*
    Assignment Number: 1
    Name: Sarita Singh
    Roll No.: 20CS10053
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <math.h>
#include <arpa/inet.h>

// function to evaluate result from operands
void evaluate(double *sum, double temp_sum, char op)
{
    switch (op)
    {
    case '+':
        *sum += temp_sum;
        break;
    case '*':
        *sum = (*sum) * temp_sum;
        break;
    case '-':
        *sum -= temp_sum;
        break;
    case '/':
        if (temp_sum != 0.000000)
            *sum /= temp_sum;
        else
            *sum = (double)INFINITY;
        break;
    }
}
double calc(char *buf_data, long long start, long long end, long long *oldi)
{
    double sum = 0, temp_sum = 0;
    char op = '?';
    // printf("\ngoing to calc with start = %d, end = %d\n", start, end);
    for (long long int i = start; i < end; i++)
    {
        if (buf_data[i] == '(')
        {
            int m = i;
            *oldi = i;
            while (buf_data[m] != ')')
                m++;

            //  if expression is of form 1+(2+3) then call the function again for 2+3 and
            //  add the value to current sum
            temp_sum = calc(buf_data, i + 1, m, oldi);
            evaluate(&sum, temp_sum, op);
            i = m+1;
            *oldi = m;
        }
        else if (i > start && (buf_data[i] == '+' || buf_data[i] == '-' || buf_data[i] == '*' || buf_data[i] == '/'))
        {
            if (*oldi == start)
                temp_sum = strtod(buf_data + *oldi, NULL);
            else
                temp_sum = strtod(buf_data + *oldi + 1, NULL);
            if (op == '?')
            {
                sum = temp_sum;
            }
            else
            {
                evaluate(&sum, temp_sum, op);
            }
            op = buf_data[i];
            *oldi = i;
            // printf("\nSum = %f   Temp sum %f *oldi = %d  \n", sum, temp_sum, oldi);
        }

        else if (buf_data[i] == ' ' || buf_data[i] == ')')
        {
            *oldi = i;
        }
    }
    // if there was no operator in the expression, then return the single operand
    if (op == '?')
    {
        sum = strtod(buf_data + start, NULL);
        return sum;
    }
    // when exp ends with operand calculuate the temp_sum for the last operand
    if (*oldi == end - 1){
        return sum;
    }
    
    temp_sum = strtod(buf_data + *oldi + 1, NULL);
    evaluate(&sum, temp_sum, op);
    return sum;
}

int main()
{
    int server_socket;
    struct sockaddr_in server_address, client_address;

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("\nError in creating socket\n");
        exit(0);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(20000);

    int clilen, error_check;
    // bind the socket to our specified IP & port information in server_address
    error_check = bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address));

    if (error_check < 0)
    {
        perror("\nError in binding\n");
        exit(0);
    }
    // 5 connection requests will be queued before further requests are refused.
    if ((error_check = listen(server_socket, 5)) < 0)
    {
        perror("\nError in listening\n");
    }
    int new_socket;

    while (1)
    {
        clilen = sizeof(client_address);
        // client socket stored in variable new_socket after accepting the connection
        if ((new_socket = accept(server_socket, (struct sockaddr *)&client_address, &clilen)) < 0)
        {
            perror("\nError in accept\n");
            exit(0);
        }

        char buffer[20];
        char *buf_data;
        buf_data = (char *)malloc(sizeof(char));
        if (buf_data == 0)
            perror("Memory exhausted");
        buf_data[0] = '\0';
        long long received, total = 0;

        while (1)
        {
            for (int i = 0; i < 20; i++)
                buffer[i] = '\0';

            // receiving in chunks untill we get '\0' to mark end of that expression
            if ((received = recv(new_socket, buffer, 20, 0)) <= 0)
            {
                strcat(buf_data, "\0");
                break;
            }
            if (total < received + strlen(buf_data))
            {
                int len = strlen(buf_data);
                total = received + len;
                buf_data = (char *)realloc(buf_data, total);
                if (buf_data == 0)
                    perror("Memory exhausted");
            }

            strcat(buf_data, buffer);
            if (buffer[received - 1] == '\0')
            {
                buffer[0] = '\0';
                break;
            }
            for (int i = 0; i < 20; i++)
                buffer[i] = '\0';
        }

        if(buf_data[0] != '\0'){
        printf("\nExpression = %s\n", buf_data);

        double sum = 0;
        long long oldi = 0;

        // call the function to evaluate the arithmatic expression
        if(buf_data[0]=='(')
        sum = calc(buf_data, 1, strlen(buf_data)-1, &oldi);
        else sum = calc(buf_data, 0, strlen(buf_data), &oldi);
        // send the result to the client
        send(new_socket, &sum, sizeof(sum), 0);
        printf("\nExpression evaluated. Result = %lf\n", sum);
        }
        free(buf_data);
        close(new_socket);
    }

    return 0;
}