// Author: Jan Lindovský

#include "lib.h"

// Constructor for sniff class
sniff::sniff(filter *fil)
{
    // Create filter expression
    string filter = fil->createFilter();
    char errorBuffer[PCAP_ERRBUF_SIZE];

    // Open network interface for packet capture
    if (fil->options.find("-i") != fil->options.end())
    {
        handle = pcap_open_live(fil->options["-i"].c_str(), BUFSIZ, 1, 1000, errorBuffer);
    }
    else
    {
        handle = pcap_open_live(fil->options["--interface"].c_str(), BUFSIZ, 1, 1000, errorBuffer);
    }

    // Check for errors in opening the interface
    if (handle == nullptr)
    {
        cerr << "Error opening interface " << fil->options["-i"] << ": " << errorBuffer << endl;
        exit(EXIT_FAILURE);
    }

    // Compile the filter expression
    struct bpf_program fp;
    if (pcap_compile(handle, &fp, filter.c_str(), 0, PCAP_NETMASK_UNKNOWN) == -1)
    {
        cerr << "Error compiling filter expression: " << pcap_geterr(handle) << endl;
        pcap_close(handle);
        exit(EXIT_FAILURE);
    }

    // Set the compiled filter
    if (pcap_setfilter(handle, &fp) == -1)
    {
        cerr << "Error setting filter: " << pcap_geterr(handle) << endl;
        pcap_close(handle);
        exit(EXIT_FAILURE);
    }
}

// Loop for capturing packets
void sniff::looper(int n)
{
    pcap_loop(handle, n, handlePacket, nullptr);
}

// Destructor for sniff class
sniff::~sniff()
{
    pcap_close(handle);
}
