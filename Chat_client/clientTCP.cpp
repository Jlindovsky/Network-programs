#include "client.h"

// tcp constructor, that create socket, hostname, ...
tcp::tcp(Parameters params)
{
    // int client_socket = sock(hostname, &server_address);
    struct sockaddr_in server_address;
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1)
    {
        std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    // Resolve hostname to IP address

    struct hostent *server_host = gethostbyname(params.hostname);
    if (server_host == NULL)
    {
        std::cerr << "Error resolving hostname: " << hstrerror(h_errno) << std::endl;
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // Server details
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(params.port);
    memcpy(&(server_address.sin_addr.s_addr), server_host->h_addr, server_host->h_length);

    if (connect(client_socket, (struct sockaddr *)(&server_address), sizeof(server_address)) == -1)
    {
        std::cerr << "Error connecting to server: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
}

// tcp destructor, sending BYE message, print terminating message to user and closing client
tcp::~tcp()
{
    sendMessage("BYE\r\n");
    std::cout << "Closing connection." << std::endl;
    close(client_socket);
}

void tcp::sendMessage(const std::string &message)
{

    if (send(client_socket, message.c_str(), message.length(), 0) == -1)
    {
        std::cerr << "ERR: sending message: " << strerror(errno) << std::endl;
        std::cout << "Closing connection." << std::endl;
        exit(EXIT_FAILURE);
    }
}

std::vector<std::string> tcp::receiveMessage()
{
    char receive_buffer[1500];
    int bytes_received = recv(client_socket, receive_buffer, sizeof(receive_buffer), 0);
    if (bytes_received == -1)
    {
        std::cerr << "ERR: receiving message: " << strerror(errno) << std::endl;
        std::cout << "Closing connection." << std::endl;
        exit(EXIT_FAILURE);
    }
    receive_buffer[bytes_received] = '\0'; // terminating nul for string

    std::vector<std::string> words; // parsed receive buffer by space will be stored there
    std::string receivedMessage = std::string(receive_buffer);

    if (!receivedMessage.empty())
    {
        std::istringstream iss(receivedMessage);
        std::string token;
        while (std::getline(iss, token, ' '))
        {
            words.push_back(token);
        }
        return words;
    }
    return words;
}

States tcp::handleAuthMessage(std::vector<std::string> words)
{
    // switch by first word
    if (words[0] == "ERR")
    {

        std::cerr << words[0] << " " << words[1] << " " << words[2] << ": " << words[4];

        return end;
    }
    else if (words[0] == "REPLY")
    {
        if (words[1] == "OK")
        {
            std::cerr << "Success:";
            for (size_t i = 3; i < words.size(); i++)
            {
                std::cerr << " " << words[i];
            }
            return open;
        }
        else
        {
            std::cerr << "Failure:";
            for (size_t i = 3; i < words.size(); i++)
            {
                std::cerr << " " << words[i];
            }
            return auth;
        }
    }
    return auth;
}

States tcp::authMessage()
{
    // check reggex for auth message
    std::regex pattern("[A-Za-z0-9-]{1,20}");
    std::regex secPattern("[A-Za-z0-9-]{1,128}");
    std::regex dNamePattern("[\x21-\x7E]{1,20}");

    std::string command;
    std::string username;
    std::string secret;
    std::string dnameI;
    std::string inputLine;
    while (true)
    {
        std::cout << " Authorize - /auth username secret displayName" << std::endl;
        std::getline(std::cin, inputLine);
        if (inputLine.empty())
        {
            return end;
        }

        std::istringstream iss(inputLine);

        if (!(iss >> command >> username >> secret >> dnameI))
        {
            std::cerr << "ERR: Insufficient arguments" << std::endl;
            return auth;
        }
        if (!std::regex_match(username, pattern))
        {
            std::cerr << "ERR: wrong username" << std::endl;
            return auth;
        }
        if (!std::regex_match(secret, secPattern))
        {
            std::cerr << "ERR: wrong secret" << std::endl;
            return auth;
        }
        if (!std::regex_match(dnameI, dNamePattern))
        {
            std::cerr << "ERR: wrong display name" << std::endl;
            return auth;
        }
        // set dname in TCP class
        dname = dnameI;
        std::string word;
        if (!(iss >> word) && command == "/auth")
        {
            std::string auth_message = "AUTH " + username + " AS " + dname + " USING " + secret + "\r\n";
            if (send(client_socket, auth_message.c_str(), auth_message.length(), 0) == -1)
            {
                std::cerr << "Error sending AUTH message: " << strerror(errno) << std::endl;
                return end;
            }

            return handleAuthMessage(receiveMessage());
        }
        else
        {
            std::cerr << "ERR: Expected exactly three words after /auth" << std::endl;
            return auth;
        }
    }
    return auth;
}

States tcp::handleRMessage(std::vector<std::string> words)
{
    // switch by first word
    if (words[0] == "MSG")
    {
        std::cout << words[2] << ":";
        for (size_t i = 4; i < words.size(); i++)
        {
            std::cout << " " << words[i];
        }
        std::cout << std::endl;
        return open;
    }
    else if (words[0] == "REPLY")
    {
        sendMessage("ERR FROM " + dname + " IS response to nothing\r\n");
        std::cerr << "ERR: got reply to nothing" << std::endl;
        return end;
    }
    else if (words[0] == "ERR")
    {
        std::cerr << words[0] << " " << words[1] << " " << words[2] << ": " << words[4];
        return end;
    }

    else if (words[0] == "BYE\n")
    {
        return end;
    }
    else
    {
        sendMessage("ERR FROM " + dname + " IS invalid message\r\n");
        std::cerr << "ERR: unreadable message" << std::endl;
        return end;
    }
}

// only receive, and not send, stored in stdin buffer
States tcp::sendJoin(const std::string &message)
{
    if (send(client_socket, message.c_str(), message.length(), 0) == -1)
    {
        std::cerr << "Error sending message: " << strerror(errno) << std::endl;
    }
    while (true)
    {
        std::vector<std::string> words = receiveMessage();
        if (!words.empty())
        {
            // check if reply or go to handle receive message
            if (words[0] == "REPLY")
            {
                if (words[1] == "OK")
                {
                    std::cerr << "Success:";
                    for (size_t i = 3; i < words.size(); i++)
                    {
                        std::cerr << " " << words[i];
                    }
                    return open;
                }
                else
                {
                    std::cerr << "Failure:";
                    for (size_t i = 3; i < words.size(); i++)
                    {
                        std::cerr << " " << words[i];
                    }
                    return open;
                }
            }
            else
            {
                States handle = handleRMessage(words);
                if (handle != open)
                {
                    return handle;
                }
            }
        }
    }
}

States tcp::openM()
{
    // regexes to join and frename commands
    std::regex pattern("[A-Za-z0-9-]{1,20}");
    std::regex dNamePattern("[\x21-\x7E]{1,20}");

    // polls for send and receive message
    struct pollfd fds[2];
    fds[0].fd = client_socket;
    fds[0].events = POLLIN;
    fds[1].fd = STDIN_FILENO;
    fds[1].events = POLLIN;
    while (true)
    {
        int ret = poll(fds, 2, -1);
        if (ret == -1)
        {
            std::cerr << "Poll error: " << strerror(errno) << std::endl;
            return end;
        }

        if (fds[0].revents & POLLIN)
        {

            std::vector<std::string> words = receiveMessage();

            if (!words.empty())
            {
                States handle = handleRMessage(words);
                if (handle != open)
                {
                    return handle;
                }
            }
        }

        if (fds[1].revents & POLLIN)
        {
            std::string inputLine;
            std::getline(std::cin, inputLine);
            if (inputLine.empty())
            {
                return end;
            }
            // switch by command, if no command send MSG
            if (inputLine.substr(0, 5) == "/join")
            {
                if (inputLine.length() > 6)
                {
                    std::string channel = inputLine.substr(6); // Start from position 6 to end
                    if (!std::regex_match(channel, pattern))
                    {
                        std::cerr << "ERR: Channel name is invalid." << std::endl;
                    }
                    else
                    {
                        States handleJoin = sendJoin("JOIN " + channel + " AS " + dname + "\r\n");
                        if (handleJoin != open)
                        {
                            return handleJoin;
                        }
                    }
                }
                else
                {
                    std::cerr << "ERR: wrong join message. Missing channel name." << std::endl;
                }
            }
            else if (inputLine.substr(0, 7) == "/rename")
            {
                if (inputLine.length() > 8)
                {
                    std::string dnameI = inputLine.substr(8); // Start from position 6 to end
                    if (!std::regex_match(dnameI, dNamePattern))
                    {
                        std::cerr << "ERR: wrong display name" << std::endl;
                    }
                    else
                    {
                        dname = dnameI;
                    }
                }
                else
                {
                    std::cerr << "ERR: wrong rename message. Misiing Display name" << std::endl;
                }
            }
            else if (inputLine == "/help")
            {
                std::cout << "Usage: ./program -t [tcp/udp] -s [hostname] -p [port] -d [timeout] -r [retransmissions] -h" << std::endl;
                std::cout << "comamnds: " << std::endl;
                std::cout << "  /join channelID - join new chennel in server" << std::endl;
                std::cout << "  /rename DisplayName - change your display name " << std::endl;
                std::cout << "  /help - print help to stardard output" << std::endl;
                std::cout << "  /auth username secret displayName - authorize conenction" << std::endl;
            }
            else if (inputLine.substr(0, 5) == "/auth")
            {
                std::cerr << "ERR: cannot send auth" << std::endl;
            }
            else
            {
                sendMessage("MSG FROM " + dname + " IS " + inputLine + "\r\n");
            }
        }
    }
    return end;
}
