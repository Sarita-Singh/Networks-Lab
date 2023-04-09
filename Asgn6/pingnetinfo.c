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

    int payloadLength = 0; // this actually decides whether the payload is sent or not
    char payload[10] = "Hello";
    strcpy(sendBuf + 28, payload);

    // init time-to-live
    int ttl = 1;
    struct timeval t1,t2;
    struct pollfd fds[1];
    fds[0].fd = sockfd;
    fds[0].events = POLLIN;

    int reached = 0;

    while(ttl <= 16) {

        float totalTime = 0, tripTime=0;

        // sending 5 packets without payload to finalize each intermediate node and n packets with payload
        for(int i = 1; i <= 5 + n; i++) {

            sleep(T);

            if(i > 5) payloadLength = strlen(payload) + 1;
            else payloadLength = 0;

            ipHeader->ttl = ttl;
            ipHeader->tot_len = 20 + 8 + payloadLength;
            ipHeader->check = 0;
            ipHeader->check= checksum((unsigned short *)ipHeader, 20);

            icmpHeader->un.echo.sequence = ttl;
            icmpHeader->un.echo.id = ttl*10 + i;
            icmpHeader->checksum = 0;
            icmpHeader->checksum = checksum((unsigned short *)(sendBuf + 20), 8 + payloadLength);  // calculating the checksum

            printf("\nFor TTL: %d\tSending ICMP packet no.:%d\n", ttl, i);
            print_header(sendBuf);

            char recvBuf[4096] = {0};
            struct sockaddr_in clientAddress;
            socklen_t len = sizeof(struct sockaddr_in);

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
                    printf("\nDestination reached: %s \t TTL: %d\t %fms\n", inet_ntoa(clientAddress.sin_addr), ttl,  (t2.tv_usec-t1.tv_usec)/1000.0);
                    reached = 1;
                }

                totalTime += tripTime;
            }
            else if (ret == 0) {
                // time exceeded
                printf("\nTime exceeded. No icmp packet received.\n");
            }
            else {
                perror("\nError in poll\n");
            }
        }

        // we calculate bandwidth as ((no of packets sent *  Length of packet in bits) / time ) 
        float bw = (5*(20 + 8) + n*(20 + 8 + strlen(payload) + 1))*8/totalTime; 
        printf("Bandwidth: %f bits/sec\n", bw);
        if(reached) break;

        ttl++;
    }

    return 0;
}