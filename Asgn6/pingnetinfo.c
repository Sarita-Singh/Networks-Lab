#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include<netinet/ip.h>
#include<netinet/ip_icmp.h>
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

// copied from internet. need to change later.
static unsigned short compute_checksum(unsigned short *addr, unsigned int count) {
  register unsigned long sum = 0;
  while (count > 1) {
    sum += * addr++;
    count -= 2;
  }
  //if any bytes left, pad the bytes and add
  if(count > 0) {
    sum += ((*addr)&htons(0xFF00));
  }
  //Fold sum to 16 bits: add carrier to result
  while (sum>>16) {
      sum = (sum & 0xffff) + (sum >> 16);
  }
  //one's complement
  sum = ~sum;
  return ((unsigned short)sum);
}

int main(int argc, char** argv) {

    // init variables
    char srcHostbuffer[256];
    struct hostent *hostEntry;
    int n, T;
    char *temp, *destIP, *srcIP;

    gethostname(srcHostbuffer, sizeof(srcHostbuffer));
    hostEntry = gethostbyname(srcHostbuffer);

    // get source IP from hostname
    temp = inet_ntoa(*((struct in_addr*)hostEntry->h_addr_list[0]));
    srcIP = (char *)malloc(strlen(temp)+1);
    strcpy(srcIP, temp);

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

    // get destination IP from hostname
    temp = inet_ntoa(*((struct in_addr*)hostEntry->h_addr_list[0]));
    destIP = (char *)malloc(strlen(temp)+1);
    strcpy(destIP, temp);

    // print init information
    printf("source IP Address: %s\n", srcIP);
    printf("destination IP Address: %s\n", destIP);
    printf("Number of probes: %d\n", n);
    printf("Time difference between each probe (in seconds): %d\n", T);

    // create raw socket for icmp
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if(sockfd == -1) {
        perror("Cannot create socket.\n");
        exit(1);
    }

    // set socket options
    int one = 1;
    if(setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
        perror("Cannot set socket options.\n");
        exit(1);
    }

    // setting up server address and port
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    inet_aton(destIP, &serverAddress.sin_addr);
    serverAddress.sin_port = htons(7); // 7 is used for echo

    // setup ip header. Copying from internet. Change later.
    char sendBuf[4096];
    struct ip *ipHeader = (struct ip *)sendBuf;
    ipHeader->ip_v = 4;
    ipHeader->ip_hl = 5;
    ipHeader->ip_tos = 0;
    ipHeader->ip_len = 20 + 8;
    ipHeader->ip_id = 10000;
    ipHeader->ip_off = 0;
    ipHeader->ip_p = IPPROTO_ICMP;
    inet_aton(srcIP, &(ipHeader->ip_src));
    net_aton(destIP, &(ipHeader->ip_dst));

    // init time-to-live
    int ttl = 1;

    while(1) {
        ipHeader->ip_ttl = ttl;
        ipHeader->ip_sum = compute_checksum((unsigned short *)ipHeader, (ipHeader->ip_hl << 2));
    }

    return 0;
}