// Author: Jan Lindovský

#include <iostream>
#include <pcap.h>
#include <string>
#include <map>
#include <arpa/inet.h>
#include <iomanip>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/if_ether.h>
#include <netinet/icmp6.h>
#include <csignal>

using namespace std;

// Function to handle captured packet
void handlePacket(unsigned char *userData, const struct pcap_pkthdr *packetHeader, const unsigned char *packetData);

// Class representing packet filter
class filter
{
public:
    map<string, string> options;
    filter(int argc, char *argv[]);
    ~filter();
    void printActiveInterfaces();
    void printHelp();
    string createFilter();
    void checkOptions(int *n);
};

// Class representing packet sniffer
class sniff
{
private:
    pcap_t *handle;

public:
    sniff(filter *fil);
    ~sniff();
    void looper(int n);
};
