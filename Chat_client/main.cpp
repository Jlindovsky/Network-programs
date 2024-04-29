#include "client.h"

// Declaration of a Protocol pointer named
Protocol *client = nullptr;

// Finite state machine
void FSM(Protocol *client)
{
    States state;
    state = auth;
    while (true)
    {
        switch (state)
        {
        case auth:
            state = client->authMessage();
            break;
        case open:
            state = client->openM();
            break;
        case end:
        {
            // exit FSM
            return;
        }
        }
    }
}

void signalHandler(int signum)
{
    // Clean up resources before exiting
    if (client != nullptr)
    {
        delete client;
        client = nullptr;
    }

    // Exit the program
    exit(signum);
}

void printHelp()
{
    std::cout << "Usage: ./program -t [tcp/udp] -s [hostname] -p [port] -d [timeout] -r [retransmissions] -h" << std::endl;
    exit(EXIT_SUCCESS);
}

int main(int argc, const char *argv[])
{
    // Ctrl+c check
    signal(SIGINT, signalHandler);

    Parameters params;
    params.tcpBool = -1; // Default value for TCP/UDP
    params.hostname = nullptr;
    params.port = 4567;         // Default port value
    params.timeout = 250;       // Default timeout value
    params.retransmissions = 3; // Default retransmissions value

    // Handle parameters
    if (argc < 2)
    {
        std::cerr << "Not enough arguments provided." << std::endl;
        printHelp();
    }

    for (int i = 1; i < argc; i += 2)
    {
        if (strcmp(argv[i], "-t") == 0)
        {
            if (i + 1 < argc)
            {
                if (strcmp(argv[i + 1], "tcp") == 0)
                {
                    params.tcpBool = 1;
                }
                else if (strcmp(argv[i + 1], "udp") == 0)
                {
                    params.tcpBool = 0;
                }
                else
                {
                    std::cerr << "Invalid argument value for -t. Use 'tcp' or 'udp'." << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                std::cerr << "No argument provided for -t option." << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        else if (strcmp(argv[i], "-s") == 0)
        {
            if (i + 1 < argc)
            {
                params.hostname = argv[i + 1];
            }
            else
            {
                std::cerr << "No argument provided for -s option." << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        else if (strcmp(argv[i], "-p") == 0)
        {
            if (i + 1 < argc)
            {
                params.port = std::stoi(argv[i + 1]);
            }
            else
            {
                std::cerr << "No argument provided for -p option." << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        else if (strcmp(argv[i], "-d") == 0)
        {
            if (i + 1 < argc)
            {
                params.timeout = std::stoi(argv[i + 1]);
            }
            else
            {
                std::cerr << "No argument provided for -d option." << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        else if (strcmp(argv[i], "-r") == 0)
        {
            if (i + 1 < argc)
            {
                params.retransmissions = std::stoi(argv[i + 1]);
            }
            else
            {
                std::cerr << "No argument provided for -r option." << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        else if (strcmp(argv[i], "-h") == 0)
        {
            printHelp();
        }
        else
        {
            std::cerr << "Invalid option: " << argv[i] << std::endl;
            printHelp();
        }
    }

    // creating client
    if (params.tcpBool == -1)
    {
        std::cout << "-t parameter not provided. \n";
        printHelp();
        exit(EXIT_FAILURE);
    }
    if (params.hostname == nullptr)
    {
        std::cout << "-s parameter not provided. \n";
        printHelp();
        exit(EXIT_FAILURE);
    }

    // creating client
    if (params.tcpBool)
    {
        client = new tcp(params);
    }
    else
    {
        client = new udp(params);
    }

    FSM(client);

    delete client;
    return 0;
}
