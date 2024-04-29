// Author: Jan Lindovský

#include "lib.h"

sniff *snif;

// Signal handler function
void signalHandler(int signum)
{
    cout << "Ctrl+C pressed. Closing pcap..." << endl;
    delete snif;
    exit(0);
}

void handlePacket(unsigned char *userData, const struct pcap_pkthdr *packetHeader, const unsigned char *packetData)
{
    // Print timestamp
    cout << "Timestamp: " << put_time(localtime(&packetHeader->ts.tv_sec), "%Y-%m-%d %H:%M:%S") << endl;

    // Extract MAC addresses
    cout << "src MAC: ";
    for (int i = 0; i < 6; ++i)
    {
        cout << hex << setw(2) << setfill('0') << static_cast<int>(packetData[i]);
        if (i < 5)
            cout << ":";
    }
    cout << endl;

    cout << "dst MAC: ";
    for (int i = 6; i < 12; ++i)
    {
        cout << hex << setw(2) << setfill('0') << static_cast<int>(packetData[i]);
        if (i < 11)
            cout << ":";
    }
    cout << endl;

    // Extract frame length
    cout << "Frame Length: " << dec << packetHeader->len << " bytes" << endl;

    uint16_t etherType = (packetData[12] << 8) | packetData[13];
    if (etherType == ETHERTYPE_IP)
    { // IPv4
        // Extract source and destination IP addresses
        cout << "src IP: " << dec << inet_ntoa(*reinterpret_cast<const in_addr *>(packetData + 26)) << endl;
        cout << "dst IP: " << dec << inet_ntoa(*reinterpret_cast<const in_addr *>(packetData + 30)) << endl;

        if (packetData[23] == 0x06)
        { // Check if TCP

            cout << "src Port: " << ntohs(*reinterpret_cast<const u_short *>(packetData + 34)) << endl;
            cout << "dst Port: " << ntohs(*reinterpret_cast<const u_short *>(packetData + 36)) << endl;
        }
        else if (packetData[23] == 0x11)
        { // Check if UDP
            cout << "src Port: " << dec << ntohs(*reinterpret_cast<const u_short *>(packetData + 34)) << endl;
            cout << "dst Port: " << dec << ntohs(*reinterpret_cast<const u_short *>(packetData + 36)) << endl;
        }
    }
    else if (etherType == ETHERTYPE_IPV6)
    {                                                                                                                   // Check if IPv6
        struct ip6_hdr *ipv6_header = reinterpret_cast<struct ip6_hdr *>(const_cast<unsigned char *>(packetData + 14)); // IPv6 header starts at byte 14
        char src_ip_str[INET6_ADDRSTRLEN];
        char dst_ip_str[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &(ipv6_header->ip6_src), src_ip_str, INET6_ADDRSTRLEN);
        inet_ntop(AF_INET6, &(ipv6_header->ip6_dst), dst_ip_str, INET6_ADDRSTRLEN);
        cout << "src IP: " << dec << src_ip_str << endl;
        cout << "dst IP: " << dec << dst_ip_str << endl;

        // Examine the next header field
        unsigned char next_header = ipv6_header->ip6_ctlun.ip6_un1.ip6_un1_nxt;
        if (next_header == IPPROTO_TCP)
        {
            cout << "src Port: " << ntohs(*reinterpret_cast<const u_short *>(packetData + 40)) << endl;
            cout << "dst Port: " << ntohs(*reinterpret_cast<const u_short *>(packetData + 42)) << endl;
        }
        else if (next_header == IPPROTO_UDP)
        {
            cout << "src Port: " << ntohs(*reinterpret_cast<const u_short *>(packetData + 40)) << endl;
            cout << "dst Port: " << ntohs(*reinterpret_cast<const u_short *>(packetData + 42)) << endl;
        }
    }
    else if (etherType == ETHERTYPE_ARP)
    { // ARP
        // Parse and handle ARP packet
        struct ether_arp *arpHeader = reinterpret_cast<struct ether_arp *>(const_cast<unsigned char *>(packetData + 14)); // ARP header starts at byte 14

        // Extract and print the source and destination IP addresses
        struct in_addr sourceIP, destIP;
        memcpy(&sourceIP, arpHeader->arp_spa, sizeof(struct in_addr));
        memcpy(&destIP, arpHeader->arp_tpa, sizeof(struct in_addr));

        cout << "src IP: " << dec << inet_ntoa(sourceIP) << endl;
        cout << "dst IP: " << dec << inet_ntoa(destIP) << endl;
    }

    // Print payload data with byte offset
    cout << endl;

    for (bpf_u_int32 i = 0; i < packetHeader->len; ++i)
    {
        if (i % 16 == 0)
        {
            cout << "0x" << hex << setw(4) << setfill('0') << i << ": ";
        }
        cout << hex << setw(2) << setfill('0') << static_cast<int>(packetData[i]) << " ";
        if (i % 8 == 7 && i % 16 != 15)
        { // Insert space in the middle of the row
            cout << " ";
        }
        if (i % 16 == 15 || i == packetHeader->len - 1)
        {
            if (i % 16 < 7)
                cout << " ";
            // align the rows
            size_t padding = (16 - (i % 16)) * 3;
            if (padding > 0)
            {
                std::cout << std::string(padding, ' ');
            }

            for (bpf_u_int32 j = i - (i % 16); j <= i; ++j)
            {
                if (j < packetHeader->len)
                {
                    if (isprint(packetData[j]))
                    {
                        cout << static_cast<char>(packetData[j]);
                    }
                    else
                    {
                        cout << ".";
                    }
                }
                else
                {
                    cout << " ";
                }
            }
            cout << endl;
        }
    }

    cout << endl
         << endl;
}
int main(int argc, char *argv[])
{
    // create filter object
    filter *fil = new filter(argc, argv);
    // count of prints
    int n = 1;

    fil->checkOptions(&n);

    // create dniff object
    sniff *snif = new sniff(fil);
    signal(SIGINT, signalHandler);

    // Start capturing packets
    snif->looper(n);
    delete snif;
    delete fil;
    return 0;
}
