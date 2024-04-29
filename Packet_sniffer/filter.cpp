// Author: Jan Lindovský

#include "lib.h"

// Constructor for the filter class
filter::filter(int argc, char *argv[])
{
    // Initialize options map with default values
    options["-i"] = "";
    options["--interface"] = "";
    options["-t"] = "";
    options["-u"] = "";
    options["-n"] = "";
    options["-p"] = "";
    options["--port-source"] = "";
    options["--port-destination"] = "";
    options["--icmp6"] = "";
    options["--tcp"] = "";
    options["--udp"] = "";
    options["--icmp4"] = "";
    options["--arp"] = "";
    options["--ndp"] = "";
    options["--igmp"] = "";
    options["--mld"] = "";
    options["-h"] = "";

    // Parse command-line options
    for (int i = 1; i < argc; i++)
    {
        string value = "";
        string key = argv[i];
        if (options.count(key) == 0)
        {
            cerr << "Error: Wrong option: " << key << endl;
            exit(EXIT_FAILURE);
        }
        if (!(key == "-i" || key == "--interface" || key == "-p" || key == "--port-source" || key == "--port-destination" || key == "-n"))
        {
            value = "true";
        }
        else
        {
            if (argc <= i + 1)
            {
                value = "true";
            }
            else
            {
                if (argv[i + 1][0] != '-')
                {
                    value = argv[i + 1];
                    i++;
                }
                else
                {
                    value = "true";
                }
            }
        }
        options[key] = value;
    }
}

// Print active network interfaces
void filter::printActiveInterfaces()
{
    // Retrieve list of active network interfaces
    struct if_nameindex *if_nidxs, *intf;
    if_nidxs = if_nameindex();
    if (if_nidxs == nullptr)
    {
        cerr << "Error: Failed to retrieve network interfaces" << endl;
        return;
    }

    cout << "List of active interfaces:" << endl;
    for (intf = if_nidxs; intf->if_name != nullptr; intf++)
    {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0)
        {
            cerr << "Error: Failed to open socket" << endl;
            continue;
        }

        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, intf->if_name, IFNAMSIZ - 1);
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0)
        {
            cerr << "Error: Failed to retrieve interface flags" << endl;
            close(sock);
            continue;
        }

        if (ifr.ifr_flags & IFF_UP)
        {
            cout << intf->if_name << endl;
        }

        close(sock);
    }

    if_freenameindex(if_nidxs);
    exit(0);
}

// Print help information
void filter::printHelp()
{
    // Print program help information with color formatting
    cout << "\033[1;34mHELP\033[0m" << endl;
    cout << "\033[1;34m-i or --interface\033[0m Network interface name for packet capture" << endl;
    cout << "\033[1;34m-h\033[0m Prints program help output and exits" << endl;
    cout << "\033[1;34m-t or --tcp\033[0m Display TCP segments and optionally complemented by -p or --port-* functionality" << endl;
    cout << "\033[1;34m-u or --udp\033[0m Display UDP datagrams and optionally complemented by -p or --port-* functionality" << endl;
    cout << "\033[1;34m-p\033[0m Extend previous two parameters to filter TCP/UDP based on port number" << endl;
    cout << "\033[1;34m--port-destination\033[0m Extend previous two parameters to filter TCP/UDP based on port number" << endl;
    cout << "\033[1;34m--port-source\033[0m Extend previous two parameters to filter TCP/UDP based on port number" << endl;
    cout << "\033[1;34m--icmp4\033[0m Display only ICMPv4 packets" << endl;
    cout << "\033[1;34m--icmp6\033[0m Display only ICMPv6 echo request/response" << endl;
    cout << "\033[1;34m--arp\033[0m Display only ARP frames" << endl;
    cout << "\033[1;34m--ndp\033[0m Display only NDP packets, subset of ICMPv6" << endl;
    cout << "\033[1;34m--igmp\033[0m Display only IGMP packets" << endl;
    cout << "\033[1;34m--mld\033[0m Display only MLD packets, subset of ICMPv6" << endl;
    cout << "\033[1;34m-n\033[0m Specify the number of packets to display, if not specified, -n 1" << endl;
    cout << "All arguments can be in any order." << endl;
    exit(0);
}

// Create packet filter expression
string filter::createFilter()
{
    string filter = "";
    string orPref = "";

    for (auto it = options.begin(); it != options.end(); ++it)
    {
        string key = it->first;
        string value = it->second;

        if (key == "-i" || key == "--interface" || key == "-n")
        {
            continue;
        }
        if (key == "-u" || key == "--udp")
        {
            filter += orPref + "udp";
        }
        else if (key == "-t" || key == "--tcp")
        {
            filter += orPref + "tcp";
        }
        else if (key == "-p")
        {
            filter += orPref + "dst port " + value + " or src port " + value;
        }
        else if (key == "--port-destination")
        {
            filter += orPref + "dst port " + value;
        }
        else if (key == "--port-source")
        {
            filter += orPref + "src port " + value;
        }
        else if (key == "--icmp4")
        {
            filter += orPref + "icmp";
        }
        else if (key == "--icmp6")
        {
            filter += orPref + "icmp6 and (icmp6[0] = 128 or icmp6[0] = 129)";
        }
        else if (key == "--arp")
        {
            filter += orPref + "arp";
        }
        else if (key == "--ndp")
        {
            filter += orPref + "ether proto 0x8ddd";
        }
        else if (key == "--igmp")
        {
            filter += orPref + "igmp";
        }
        else if (key == "--mld")
        {
            filter += orPref + "ip6 multicast";
        }
        orPref = " or ";
    }

    return filter;
}

// Validate command-line options and set packet count limit if specified
void filter::checkOptions(int *n)
{
    if (options.at("-h") == "true")
    {
        printHelp();
    }
    if (options.at("-i") == "" && options.at("--interface") == "" || options.at("-i") != "" && options.at("--interface") != "")
    {
        cerr << "Error: Please specify the network interface with the '-i' or '--interface' option." << endl;
        exit(1);
    }
    if ((options.at("-t") != "" && options.at("--tcp") != "") || (options.at("-u") != "" && options.at("--udp") != ""))
    {
        cerr << "Error: Please specify the connection with the '-t','--tcp','-u','--udp' option." << endl;
        exit(1);
    }
    if (options.at("-n") != "")
    {
        try
        {
            *n = stoi(options.at("-n"));
        }
        catch (invalid_argument const &ex)
        {
            cerr << "Invalid value for -n" << endl;
            exit(1);
        }
    }

    // Remove unused flags from map
    auto it = options.begin();
    while (it != options.end())
    {
        if (it->second == "")
        {
            it = options.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

filter::~filter()
{
}
