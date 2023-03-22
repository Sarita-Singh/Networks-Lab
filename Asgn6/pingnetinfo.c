#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Prints how the program should be run
void printHelp() {
    printf("\nPlease use the following format to run PingNetInfo.\n");
    printf("\n./PingNetInfo <hostname> <n> <T>\n");
    printf("\nArgument details is given below.\n");
    printf(" -> hostname: Name of the host like 'cse.iitkgp.ac.in' or an IP address.\n");
    printf(" -> n: The number of times a probe will be sent per link.\n");
    printf(" -> T: The time difference between any two probes (seconds).\n");
}

int main(int argc, char** argv) {

    // init variables
    struct hostent* hostEntry;
    int n, T;
    char *IPbuffer;

    // processing arguments
    if(argc < 4) {
        printHelp();
        return 0;
    }
    n = atoi(argv[2]);
    T = atoi(argv[3]);
    hostEntry = gethostbyname(argv[1]);

    if(hostEntry == NULL) {
        printf("Host not found or could not be reached.\n");
        return 0;
    }

    // get IP from hostname
    IPbuffer = inet_ntoa(*((struct in_addr*)hostEntry->h_addr_list[0]));

    // print init information
    printf("IP Address: %s\n", IPbuffer);
    printf("Number of probes: %d\n", n);
    printf("Time difference between each probe (in seconds): %d\n", T);

    return 0;
}