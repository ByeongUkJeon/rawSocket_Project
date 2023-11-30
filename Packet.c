/*  C ���� �ۼ��� ��Ŷ ĸó ���α׷��Դϴ�.
      - ICMP, DNS, HTTP, SSH 4�� �������� �����Դϴ�.
      - raw socket�� ���� Network Device�κ��� ��Ŷ or �����͸� �����մϴ�.
      - ������ �ü���� ������� �۵��մϴ�.

    Network Ʈ������ �����ϰ� Ư�� �������ݿ� ���� ���� ������ �α� ���Ͽ� �����ϴ� ����� ������ �ֽ��ϴ�.
*/

// ������� ����
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <netinet/ip_icmp.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

// ĸ�ĵ� ��Ŷ�� �����ϴµ� ���Ǵ� ������ ũ�⸦ 65536���� ����
#define BUFFER_SIZE 65536

//�������� ����
FILE* logfile;                          // ĸ�ĵ� ��Ŷ ������ �α� ���Ͽ� ���� ���� ���� ������
int sock_raw;                           // raw socket�� socket discriptor
struct sockaddr_in source, dest;        // �۽����� �������� IP �ּҸ� �����ϱ� ���� ����ü
int myflag = 0;

void ProcessPacket(unsigned char*, int, char*);     // ��Ŷ�� ó���ϰ� 4�� ��������(ICMP, DNS, HTTP, SSH)�� �´� �Լ��� ȣ���ϴ� �Լ�
void LogIcmpPacket(unsigned char*, int, char*);     // ICMP ��Ŷ ���� ������ ����ϴ� �Լ�
void LogDnsPacket(unsigned char*, int, char*);      // DNS ��Ŷ ���� ������ ����ϴ� �Լ�
void LogHttpPacket(unsigned char*, int, char*);     // HTTP ��Ŷ ���� ������ ����ϴ� �Լ�
void LogSshPacket(unsigned char*, int, char*);      // SSH ��Ŷ ���� ������ ����ϴ� �Լ�
void LogIpHeader(unsigned char*, int, char*);       // IP ��� ���� ������ ����ϴ� �Լ�
void LogData(unsigned char*, int);                  // ������ ���̷ε� ���� ������ ����ϴ� �Լ�



// ��Ŷ�� ó���ϰ� 4�� ��������(ICMP, DNS, HTTP, SSH)�� �´� �Լ��� ȣ���ϴ� �Լ�
void ProcessPacket(unsigned char* buffer, int size, char* pip_so)
{
    struct iphdr* iph = (struct iphdr*)(buffer + sizeof(struct ethhdr));

    switch (iph->protocol) {
    // ICMP�� 1�� ��Ʈ�� �̿��Ѵ�
    case 1:     
        LogIcmpPacket(buffer, size, pip_so);
        printf("ICMP Packet Captured\t");
        break;
    // TCP�� 6�� ��Ʈ�� �̿��Ѵ�
    case 6:
        // TCP�� ����ϴ� HTTP�� 80�� ��Ʈ�� �̿��Ѵ�
        struct tcphdr* tcph = (struct tcphdr*)(buffer + sizeof(struct ethhdr) + iph->ihl * 4);
        if (ntohs(tcph->source) == 80 || ntohs(tcph->dest) == 80) {
            LogHttpPacket(buffer, size, pip_so);
            printf("HTTP Packet Captured\t");
        }

        // TCP�� ����ϴ� SSH Protocol�� 22�� ��Ʈ�� �̿��Ѵ�
        if (ntohs(tcph->source) == 22 || ntohs(tcph->dest) == 22) {
            LogSshPacket(buffer, size, pip_so);
            printf("SSH Packet Captured\t");
        }
        break;

    //DNS Protocol�� 17�� ��Ʈ�� �̿��Ѵ�
    case 17:
        LogDnsPacket(buffer, size, pip_so);
        printf("DNS Packet Captured\t");
        break;
    default:
        printf("Other Packet Captured\t");
    }
}

// ICMP ��Ŷ ���� ������ ����ϴ� �Լ�
void LogIcmpPacket(unsigned char* buffer, int size, char* pip_so)
{
    // IP ������� ICMP ����� �̵�
    struct iphdr* iph = (struct iphdr*)(buffer + sizeof(struct ethhdr));
    unsigned short iphdrlen = iph->ihl * 4;

    struct icmphdr* icmph = (struct icmphdr*)(buffer + iphdrlen + sizeof(struct ethhdr));

    // ICMP ��Ŷ ����� ũ��
    int header_size = sizeof(struct ethhdr) + iphdrlen + sizeof(struct icmphdr);

    // ICMP ��Ŷ �м� ���� �α� ����
    fprintf(logfile, "\n\n- - - - - - - - - - - ICMP Packet - - - - - - - - - - - - \n");

    LogIpHeader(buffer, size, pip_so);

    fprintf(logfile, "\nICMP Header\n");
    fprintf(logfile, " + Type                 : %u\n", (unsigned int)(icmph->type));
    fprintf(logfile, " | Code                 : %u\n", (unsigned int)(icmph->code));
    fprintf(logfile, " | Checksum             : %d\n", ntohs(icmph->checksum));
    fprintf(logfile, " + Identifier           : %u\n", ntohs(icmph->un.echo.id));
    fprintf(logfile, " | Sequence Number      : %u\n", ntohs(icmph->un.echo.sequence));

    // IP ��� ������ �α�
    fprintf(logfile, "\n");
    fprintf(logfile, "IP Header\n");
    LogData(buffer, iphdrlen);

    // ICMP ��� ������ �α� 
    fprintf(logfile, "\nICMP Header\n");
    LogData(buffer + iphdrlen, sizeof(struct icmphdr));

    // ICMP ������ ���̷ε� �α�
    fprintf(logfile, "\nData Payload\n");
    LogData(buffer + header_size, size - header_size);

    // ICMP ��Ŷ �α� ����
    fprintf(logfile, "\n- - - - - - - - - - - - - - - - - - - - - -");
}

// DNS Protocol ��� ������ �����ϴ� ����ü
struct dnshdr {
    uint16_t id;                // DNS Protocol ��Ŷ �ĺ��� (Identification)
    uint16_t flags;             // DNS Flags - �������� ���� ������ ��� ����
    uint16_t questions;         // DNS ������ ���� 
    uint16_t answer_rrs;        // DNS ���� ���ڵ��� ����
    uint16_t authority_rrs;     // DNS ���� ���ڵ��� ����
    uint16_t additional_rrs;    // DNS �߰� ���ڵ��� ����
};

// DNS Protocol ��Ŷ ���� ������ ����ϴ� �Լ�
void LogDnsPacket(unsigned char* buffer, int size, char* pip_so)
{
    // IP ��� ������ �о��
    struct iphdr* iph = (struct iphdr*)(buffer + sizeof(struct ethhdr));
    unsigned short iphdrlen = iph->ihl * 4;

    // UDP ��� ������ �о��
    struct udphdr* udph = (struct udphdr*)(buffer + iphdrlen + sizeof(struct ethhdr));

    // ����� ��ü ũ�� ��� (Ethernet ��� + IP ��� + UDP ���)
    int header_size = sizeof(struct ethhdr) + iphdrlen + sizeof(struct udphdr);

    // DNS Protocol ��Ŷ �α� ����
    fprintf(logfile, "\n\n- - - - - - - - - - - DNS Packet - - - - - - - - - - - - \n");

    // IP ��� ���� �α�
    LogIpHeader(buffer, size, pip_so);

    // UDP ��� ���� �α�
    fprintf(logfile, "\nUDP Header\n");
    fprintf(logfile, " + Source Port          : %d\n", ntohs(udph->source));
    fprintf(logfile, " | Destination Port     : %d\n", ntohs(udph->dest));
    fprintf(logfile, " | UDP Length           : %d\n", ntohs(udph->len));
    fprintf(logfile, " + UDP Checksum         : %d\n", ntohs(udph->check));

    // DNS ��� ���� �α�
    fprintf(logfile, "\nDNS Header\n");
    struct dnshdr* dnsh = (struct dnshdr*)(buffer + header_size);
    fprintf(logfile, " + Transaction ID       : %d\n", ntohs(dnsh->id));
    fprintf(logfile, " | Flags                : %d\n", ntohs(dnsh->flags));
    fprintf(logfile, " | Questions            : %d\n", ntohs(dnsh->questions));
    fprintf(logfile, " | Answer RRs           : %d\n", ntohs(dnsh->answer_rrs));
    fprintf(logfile, " | Authority RRs        : %d\n", ntohs(dnsh->authority_rrs));
    fprintf(logfile, " + Additional RRs       : %d\n", ntohs(dnsh->additional_rrs));

    // DNS ������ ���̷ε� �α�
    fprintf(logfile, "\nData Payload\n");
    LogData(buffer + header_size + sizeof(struct dnshdr), size - header_size - sizeof(struct dnshdr));

    // DNS ��Ŷ �α� ����
    fprintf(logfile, "\n- - - - - - - - - - - - - - - - - - - - - -");
}

// IP ��� ���� ������ ����ϴ� �Լ�
void LogIpHeader(unsigned char* buffer, int size, char* pip_so)
{
    unsigned short iphdrlen;

    // IP ����� ���� ��ġ�� ���
    struct iphdr* iph = (struct iphdr*)(buffer + sizeof(struct ethhdr));
    iphdrlen = iph->ihl * 4;

    // �۽��� IP �ּ� �ʱ�ȭ �� ����
    memset(&source, 0, sizeof(source));
    iph->saddr = inet_addr(pip_so);
    source.sin_addr.s_addr = iph->saddr;//ip�� �޾ƿ´�.

    // ������ IP �ּ� �ʱ�ȭ �� ����
    memset(&dest, 0, sizeof(dest));
    dest.sin_addr.s_addr = iph->daddr;

    // IP ��� ���� �α�
    fprintf(logfile, "\n");
    fprintf(logfile, "IP Header\n");
    fprintf(logfile, " + IP Version          : %d\n", (unsigned int)iph->version);
    fprintf(logfile, " | IP Header Length    : %d Bytes\n", ((unsigned int)(iph->ihl)) * 4);
    fprintf(logfile, " | Type Of Service     : %d\n", (unsigned int)iph->tos);
    fprintf(logfile, " | IP Total Length     : %d  Bytes (FULL SIZE)\n", ntohs(iph->tot_len));
    fprintf(logfile, " | TTL                 : %d\n", (unsigned int)iph->ttl);
    fprintf(logfile, " | Protocol            : %d\n", (unsigned int)iph->protocol);
    fprintf(logfile, " | Checksum            : %d\n", ntohs(iph->check));
    fprintf(logfile, " | Source IP           : %s\n", inet_ntoa(source.sin_addr));
    fprintf(logfile, " + Destination IP      : %s\n", inet_ntoa(dest.sin_addr));
}

// HTTP ��Ŷ ��� ������ ����ϴ� �Լ�
void LogHttpPacket(unsigned char* buffer, int size, char* pip_so)
{
    // HTTP ��Ŷ �α� ����
    fprintf(logfile, "\n\n- - - - - - - - - - - HTTP Packet - - - - - - - - - - - - \n");

    // Ethernet ��� ���� �α�
    struct ethhdr* eth = (struct ethhdr*)buffer;
    fprintf(logfile, "\nEthernet Header\n");
    fprintf(logfile, " + Source MAC: %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",
        eth->h_source[0], eth->h_source[1], eth->h_source[2],
        eth->h_source[3], eth->h_source[4], eth->h_source[5]);
    fprintf(logfile, " + Destination MAC: %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",
        eth->h_dest[0], eth->h_dest[1], eth->h_dest[2],
        eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]);
    fprintf(logfile, " + Protocol: %u\n", (unsigned short)eth->h_proto);

    // IP ��� ���� �α�
    struct iphdr* iph = (struct iphdr*)(buffer + sizeof(struct ethhdr));
    unsigned short iphdrlen = iph->ihl * 4;
    fprintf(logfile, "\nIP Header\n");
    fprintf(logfile, " + Source IP: %s\n", inet_ntoa(source.sin_addr));
    fprintf(logfile, " + Destination IP: %s\n", inet_ntoa(dest.sin_addr));
    fprintf(logfile, " + Protocol: %u\n", (unsigned int)iph->protocol);
    
    // TCP ��� ���� �α�
    struct tcphdr* tcph = (struct tcphdr*)(buffer + iphdrlen + sizeof(struct ethhdr));
    fprintf(logfile, "\nTCP Header\n");
    fprintf(logfile, " + Source Port: %u\n", ntohs(tcph->source));
    fprintf(logfile, " + Destination Port: %u\n", ntohs(tcph->dest));
    fprintf(logfile, " + Sequence Number: %u\n", ntohl(tcph->seq));
    fprintf(logfile, " + Acknowledge Number: %u\n", ntohl(tcph->ack_seq));
    fprintf(logfile, " + Header Length: %d BYTES\n", (unsigned int)tcph->doff * 4);
    fprintf(logfile, " + Acknowledgement Flag: %d\n", (unsigned int)tcph->ack);
    fprintf(logfile, " + Finish Flag: %d\n", (unsigned int)tcph->fin);
    fprintf(logfile, " + Checksum: %d\n", ntohs(tcph->check));
    
    // HTTP ������ ���̷ε� �α�
    fprintf(logfile, "\nData Payload\n");
    LogData(buffer + iphdrlen + sizeof(struct ethhdr) + sizeof(struct tcphdr), size - (iphdrlen + sizeof(struct ethhdr) + sizeof(struct tcphdr)));

    // HTTP ��Ŷ �α� ����
    fprintf(logfile, "\n- - - - - - - - - - - - - - - - - - - - - -");
}

// SSH ��Ŷ ��� ������ ����ϴ� �Լ�
void LogSshPacket(unsigned char* buffer, int size, char* pip_so)
{
    // SSH ��Ŷ �α� ����
    fprintf(logfile, "\n\n- - - - - - - - - - - SSH Packet - - - - - - - - - - - - \n");

    // Ethernet ��� ���� �α�
    struct ethhdr* eth = (struct ethhdr*)buffer;
    fprintf(logfile, "\nEthernet Header\n");
    fprintf(logfile, " + Source MAC: %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",
        eth->h_source[0], eth->h_source[1], eth->h_source[2],
        eth->h_source[3], eth->h_source[4], eth->h_source[5]);
    fprintf(logfile, " + Destination MAC: %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",
        eth->h_dest[0], eth->h_dest[1], eth->h_dest[2],
        eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]);
    fprintf(logfile, " + Protocol: %u\n", (unsigned short)eth->h_proto);

    // IP ��� ���� �α�
    struct iphdr* iph = (struct iphdr*)(buffer + sizeof(struct ethhdr));
    unsigned short iphdrlen = iph->ihl * 4;
    fprintf(logfile, "\nIP Header\n");
    fprintf(logfile, " + Source IP: %s\n", inet_ntoa(source.sin_addr));
    fprintf(logfile, " + Destination IP: %s\n", inet_ntoa(dest.sin_addr));
    fprintf(logfile, " + Protocol: %u\n", (unsigned int)iph->protocol);

    // TCP ��� ���� �α�
    struct tcphdr* tcph = (struct tcphdr*)(buffer + iphdrlen + sizeof(struct ethhdr));
    fprintf(logfile, "\nTCP Header\n");
    fprintf(logfile, " + Source Port: %u\n", ntohs(tcph->source));
    fprintf(logfile, " + Destination Port: %u\n", ntohs(tcph->dest));
    fprintf(logfile, " + Sequence Number: %u\n", ntohl(tcph->seq));
    fprintf(logfile, " + Acknowledge Number: %u\n", ntohl(tcph->ack_seq));
    fprintf(logfile, " + Header Length: %d BYTES\n", (unsigned int)tcph->doff * 4);
    fprintf(logfile, " + Acknowledgement Flag: %d\n", (unsigned int)tcph->ack);
    fprintf(logfile, " + Finish Flag: %d\n", (unsigned int)tcph->fin);
    fprintf(logfile, " + Checksum: %d\n", ntohs(tcph->check));

    // SSH ������ ���̷ε� �α�
    fprintf(logfile, "\nData Payload\n");
    LogData(buffer + iphdrlen + sizeof(struct ethhdr) + sizeof(struct tcphdr), size - (iphdrlen + sizeof(struct ethhdr) + sizeof(struct tcphdr)));

    // SSH ��Ŷ �α� ����
    fprintf(logfile, "\n- - - - - - - - - - - - - - - - - - - - - -");
}

// ������ ���̷ε� ���� ������ ����ϴ� �Լ�
void LogData(unsigned char* buffer, int size)
{
    int i, j;
    for (i = 0; i < size; i++) { 
        // ������ ���̷ε�� �� �ٿ� 16����Ʈ�� ���
        // ���� �� �ٿ� 16����Ʈ�� ��µǵ��� ����
        if (i != 0 && i % 16 == 0) { 

            for (j = i - 16; j < i; j++) {
                if (buffer[j] >= 32 && buffer[j] <= 128) {
                    fprintf(logfile, " %c", (unsigned char)buffer[j]);
                }
                else {
                    // ������ ���̷ε� ����Ʈ�� ��������� ���� ó��
                    fprintf(logfile, " *");
                }
            }
            // 16����Ʈ�� ��� ����� ���� ���� �ٷ� �̵�
            fprintf(logfile, "\t\n");
        }

        if (i % 16 == 0) {
            fprintf(logfile, " ");
        }
        // ������ ���̷ε� ��Ŀ� �°� 2�ڸ� 16������ ǥ���Ͽ� ����Ʈ �ڵ� ���
        // �������� �������� ���̱� ���� ���� �ڵ�
        fprintf(logfile, " %02X", (unsigned int)buffer[i]);

        if (i == size - 1) {
            for (j = 0; j < 15 - i % 16; j++) {
                // ������ ���� ���ĵ� ����� �����ϱ� ���� ���� �߰�
                fprintf(logfile, "  "); 
            }

            // ������ ���� ���� ���
            for (j = i - i % 16; j <= i; j++) {
                if (buffer[j] >= 32 && buffer[j] <= 128) {
                    fprintf(logfile, " %c", (unsigned char)buffer[j]);
                }
                else {
                    fprintf(logfile, " *");
                }
            }

            fprintf(logfile, "\n");
        }
    }
}

int main(int argc, char* argv[])
{
    char ip_source[18];
    char* pip_so = ip_source;
    char num_port[7];
    char* p_port = num_port;

    printf("+------ Packet Capture Program -------+\n");

    strcpy(p_port, argv[1]);
    printf("| Entered port:   %s\n", p_port);

    strcpy(pip_so, argv[2]);
    printf("| Entered ip:   %s\n", pip_so);

    printf("+--------------------------------+\n");

    socklen_t saddr_size;
    int data_size;
    struct sockaddr saddr;
    struct in_addr in;

    unsigned char* buffer = (unsigned char*)malloc(BUFFER_SIZE);

    // ����ڰ� ������ Protocol�� ����Ͽ� �α� ������ ���� ��Ŷ ������ �ۼ�
    if (!strcmp(p_port, "icmp")) {
        logfile = fopen("log_icmp.txt", "w");
        printf("log_icmp.txt Writing\n");
        if (logfile == NULL) {
            printf("icmp log file create failed\n");
            return 1;
        }
    }

    else if (!strcmp(p_port, "dns")) {
        myflag = 1;
        logfile = fopen("log_dns.txt", "w");
        printf("log_dns.txt Writing\n");
        if (logfile == NULL) {
            printf("dns log file create failed \n");
            return 1;
        }
    }
    else if (!strcmp(p_port, "http")) {
        logfile = fopen("log_http.txt", "w");
        printf("log_http.txt Writing\n");
        if (logfile == NULL) {
            printf("http log file create failed \n");
            return 1;
        }
    }
    else if (!strcmp(p_port, "ssh")) {
        logfile = fopen("log_ssh.txt", "w");
        printf("log_ssh.txt Writing\n");
        if (logfile == NULL) {
            printf("ssh log file create failed \n");
            return 1;
        }
    }
    
    else {
        printf("Unknown Error \n");
        return 1;
    }
    // AF_PACKET, SOCK_RAW �� ����Ͽ� ���� �ʱ�ȭ (Layer 2���� ���� ����)
    sock_raw = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock_raw < 0) {
        printf("socket init failed\n");
        return 1;
    }

    while (1) {
        saddr_size = sizeof saddr;

        data_size = recvfrom(sock_raw, buffer, BUFFER_SIZE, 0, &saddr, &saddr_size);
        if (data_size < 0) {
            printf("return is smaller than 0");
            return 1;
        }

        ProcessPacket(buffer, data_size, pip_so);
    }

    close(sock_raw);

    return 0;
}