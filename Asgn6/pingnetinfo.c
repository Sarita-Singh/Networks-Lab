#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <netinet/in.h>
#include<netinet/ip.h>
#include<netinet/ip_icmp.h>
#include <arpa/inet.h>

int reached = 0;

// Prints how the program should be run
void printHelp() {
    printf("\nPlease use the following format to run PingNetInfo.\n");
    printf("\n./PingNetInfo <hostname> <n> <T>\n");
    printf("\nArgument details is given below.\n");
    printf(" -> hostname: Name of the host like 'cse.iitkgp.ac.in' or an IP address.\n");
    printf(" -> n: The number of times a probe will be sent per link.\n");
    printf(" -> T: The time difference between any two probes (seconds).\n");
}

unsigned short checksum(unsigned short *addr, int len) {
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2) {
        sum += *addr++;
    }
    if (len == 1) {
        sum += *(unsigned char *)addr;
    }
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

void print_header(char *recvbuffer) {
    struct icmphdr *recv_icmphdr = (struct icmphdr *)(recvbuffer + 20);

    printf("ICMP type: %d\n", recv_icmphdr->type);
    printf("ICMP code: %d\n", recv_icmphdr->code);
    printf("ICMP echo id: %d\n", recv_icmphdr->un.echo.id);
    printf("ICMP echo sequence: %d\n", recv_icmphdr->un.echo.sequence);
    printf("ICMP gateway address: %d\n", recv_icmphdr->un.gateway);
}

float sendPackets(char *payload, int ttl, char *sendBuf, int sockfd, int echoID, struct sockaddr_in serverAddress) {
    struct timeval t1,t2;

    strcpy(sendBuf + 28, payload);
    int payloadLength = strlen(payload) + 1;
    struct iphdr *ipHeader = (struct iphdr *)sendBuf;
    struct icmphdr *icmpHeader = (struct icmphdr *)(sendBuf + 20);

    struct pollfd fds[1];
    fds[0].fd = sockfd;
    fds[0].events = POLLIN;

    ipHeader->ttl = ttl;
    ipHeader->tot_len = 20 + 8 + payloadLength;
    ipHeader->check = 0;
    ipHeader->check= checksum((unsigned short *)ipHeader, 20);

    icmpHeader->un.echo.sequence = ttl;
    icmpHeader->un.echo.id = echoID;
    icmpHeader->checksum = 0;
    icmpHeader->checksum = checksum((unsigned short *)(sendBuf + 20), 8 + payloadLength);  // calculating the checksum

    printf("\nFor TTL: %d\tSending ICMP packet id:%d\n", ttl, echoID);
    print_header(sendBuf);

    char recvBuf[4096] = {0};
    struct sockaddr_in clientAddress;
    socklen_t len = sizeof(struct sockaddr_in);
    float tripTime;
    sendto(sockfd, sendBuf, sizeof(struct iphdr) + sizeof(struct icmphdr) + payloadLength, 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    gettimeofday(&t1, NULL);

    int ret = poll(fds, 1, 2000);   // poll with timeout of 2 sec

    if(ret > 0) {
        recvfrom(sockfd, recvBuf, sizeof(recvBuf), 0, (struct sockaddr *)&clientAddress, &len);
        gettimeofday(&t2, NULL);

        tripTime = (t2.tv_sec-t1.tv_sec)*1000.0 + (t2.tv_usec-t1.tv_usec)/1000.0;

        struct icmphdr *icmpRecvHeader = (struct icmphdr *) (recvBuf + 20);

        printf("\nReceived an ICMP packet.\n");
        print_header(recvBuf);

        if(icmpRecvHeader->type == 11) {
            printf("\nTTL: %d \t Address: %s\t %fms\n", ttl, inet_ntoa(clientAddress.sin_addr), tripTime);
        }
        else if(icmpRecvHeader->type == 0) {
            printf("\nDestination reached: %s \t TTL: %d\t %fms\n", inet_ntoa(clientAddress.sin_addr), ttl,  tripTime);
            reached = 1;
        }
    }
    else if (ret == 0) {
        // time exceeded
        printf("\nTime exceeded. No icmp packet received.\n");
    }
    else {
        perror("\nError in poll\n");
    }

    return tripTime;
}

int main(int argc, char** argv) {

    // init variables
    char srcHostbuffer[256];
    struct hostent *hostEntry;
    int n, T;
    char *temp, *destIP, *srcIP;
    printf("srchost buf: %s\n", srcHostbuffer);
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
    serverAddress.sin_port = htons(36969); 

    // setup ip headers
    char sendBuf[4096] = {0};
    struct iphdr *ipHeader = (struct iphdr *)sendBuf;
    ipHeader->version = 4;
    ipHeader->ihl = 5;
    ipHeader->tos = 0;
    ipHeader->id = 10000;
    ipHeader->frag_off = 0;
    ipHeader->protocol = IPPROTO_ICMP;
    inet_aton(srcIP, &(ipHeader->saddr));
    inet_aton(destIP, &(ipHeader->daddr));

    // setup icmp headers
    struct icmphdr *icmpHeader = (struct icmphdr *)(sendBuf + 20);
    icmpHeader->type = ICMP_ECHO;
    icmpHeader->code = 0;

    char payload[20] = "Hello World!";

    // init time-to-live
    int ttl = 1;

    float prevTimeWithData = 0, prevTimeWithoutData = 0;

    while(ttl <= 16) {

        // sending 5 packets without payload to finalize each intermediate node and n packets with payload
        /*
            5 packets without data to finalize each intermediate node 
            n packets with 0 data and n packs with some data
            calculate avg RTT of data packs for each link and av RTT of header packs
            divide av of data by av time
        */

        for(int i = 1; i <= 5; i++) {
            sendPackets("", ttl, sendBuf, sockfd, ttl*10 + i, serverAddress);
        }

        float timeWithData = 0, timeWithoutData = 0;

        // without data
        for(int i = 1; i <= n; i++) {
            timeWithoutData += sendPackets("", ttl, sendBuf, sockfd, ttl*10 + i, serverAddress);
            sleep(T);
        }

        // with data
        for(int i = 1; i <= n; i++) {
            timeWithData += sendPackets(payload, ttl, sendBuf, sockfd, ttl*10 + i, serverAddress);
            sleep(T);
        }

        // we calculate bandwidth as ((no of packets sent *  Length of packet in bits) / time ) 
        float avgLinkTimeWithoutData = (timeWithoutData - prevTimeWithoutData)/n;
        float avgLinkTimeWithData = (timeWithData - prevTimeWithData)/n;
        float bw = ((strlen(payload) + 1) * 8)/(avgLinkTimeWithData - avgLinkTimeWithoutData); 
        printf("Bandwidth: %f bits/sec\n", bw);

        if(reached) break;

        prevTimeWithData = timeWithData;
        prevTimeWithoutData = timeWithoutData;

        ttl++;
    }

    return 0;
}