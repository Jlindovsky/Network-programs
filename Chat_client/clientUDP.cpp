#include "client.h"

// udp constructor, that create socket, hostname, ...
udp::udp(Parameters params)
{
    // set atribute timeOut value
    timeOut = params.timeout;
    // set atribute value
    retransmissions = params.retransmissions + 1;
    // set atribute value
    messageId = 0;
    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket == -1)
    {
        std::cerr << "Error creating UDP socket: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(params.port);

    struct hostent *host = gethostbyname(params.hostname);
    if (host == nullptr)
    {
        std::cerr << "Error resolving hostname: " << hstrerror(h_errno) << std::endl;
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    memcpy(&(server_address.sin_addr.s_addr), host->h_addr, host->h_length);
    server_address_size = sizeof(server_address);
}

// udp destructor, sending BYE message, print terminating message to user and closing client
udp::~udp()
{

    struct pollfd fds[1];
    fds[0].fd = client_socket;
    fds[0].events = POLLIN;

    uint8_t message_type = 0xFF;
    uint16_t messageId = this->messageId;
    size_t message_length = 1 + 2;
    std::vector<uint8_t> message_buffer(message_length);
    size_t offset = 0;
    message_buffer[offset++] = message_type;
    message_buffer[offset++] = messageId >> 8;
    message_buffer[offset++] = messageId & 0xFF;
    messageId++;
    for (int i = 0; i < this->retransmissions; ++i)
    {
        ssize_t bytesSent = sendto(client_socket, message_buffer.data(), message_buffer.size(), 0, (struct sockaddr *)&server_address, server_address_size);
        if (bytesSent == -1)
        {
            std::cerr << "ERR: msg send failed" << strerror(errno) << std::endl;
            break;
        }

        int pollR = poll(fds, 1, this->timeOut);
        if (pollR == -1)
        {
            std::cerr << "ERROR: " << strerror(errno) << std::endl;
            break;
        }

        if (pollR > 0 && fds[0].revents & POLLIN)
        {
            auto received = receiveMessage();
            uint8_t type = received[0];
            if (type == 0x00 && (((received[1] << 8) | received[2]) == messageId))
                break;
        }
    }

    std::cout << "Closing connection." << std::endl;
    close(client_socket);
}

bool udp::sendMessage(UDPpckt &pckt)
{
    // check for resending message
    if (pckt.PCKTretransmissions != 0)
    {
        pckt.PCKTretransmissions--;
        ssize_t bytesSent = sendto(client_socket, pckt.message_buffer.data(), pckt.message_buffer.size(), 0, (struct sockaddr *)&server_address, server_address_size);
        if (bytesSent == -1)
        {
            std::cerr << "ERR: msg send failed" << strerror(errno) << std::endl;
            return false;
        }
        pckt.sentTime = std::chrono::steady_clock::now();
        return true;
    }
    else
    {
        std::cerr << "ERR: msg send failed because too many retries" << std::endl;

        return false;
    }
}

std::vector<char> udp::receiveMessage()
{
    const size_t receive_buffer_size = 1500; // size to fit 1400 characters in message
    std::vector<char> receive_buffer(receive_buffer_size);

    // Receive message from the server
    ssize_t bytesReceived = recvfrom(client_socket, receive_buffer.data(), receive_buffer.size(), 0,
                                     (struct sockaddr *)&server_address, &server_address_size);
    if (bytesReceived == -1)
    {
        std::cerr << "ERR: Failed to receive message" << std::endl;
        exit(EXIT_FAILURE);
    }
    else if (bytesReceived == 0)
    {
        std::cerr << "ERR: Connection closed by server" << std::endl;
        exit(EXIT_FAILURE);
    }

    uint8_t type = receive_buffer[0];
    // cinfirm doesn't have confirm on itself
    if (type != 0x00)
    {
        size_t message_length = 3;
        std::vector<uint8_t> message_buffer(message_length);
        message_buffer[0] = 0x00;
        // creating ID
        message_buffer[1] = receive_buffer[1] >> 8;
        message_buffer[2] = receive_buffer[2] & 0xFF;
        ssize_t bytesSent = sendto(client_socket, message_buffer.data(), message_buffer.size(), 0, (struct sockaddr *)&server_address, server_address_size);
        if (bytesSent == -1)
        {
            std::cerr << "ERR: msg send failed" << strerror(errno) << std::endl;
            exit(EXIT_FAILURE);
        }
        return receive_buffer;
    }
    return receive_buffer;
}

States udp::replyM(UDPpckt &pckt, bool authBool)
{
    // poll for timeout
    struct pollfd fds;
    fds.fd = client_socket;
    fds.events = POLLIN;
    // sending retransmission times
    for (int i = 0; i < retransmissions; i++)
    {
        ssize_t bytesSent = sendto(client_socket, pckt.message_buffer.data(), pckt.message_buffer.size(), 0, (struct sockaddr *)&server_address, server_address_size);
        if (bytesSent == -1)
        {
            std::cerr << "ERR: msg send failed" << strerror(errno) << std::endl;
            exit(EXIT_FAILURE);
        }
        while (true)
        {
            int pollR = poll(&fds, 1, timeOut);
            if (pollR == -1)
            {
                std::cerr << "ERROR: " << strerror(errno) << std::endl;
                exit(EXIT_FAILURE);
            }
            if (pollR == 0)
            {
                break;
            }
            if (fds.revents & POLLIN)
            {
                auto received = receiveMessage();
                // check receive message type, OK/NOK, refID
                if (received[0] == 0x01 && received[3] == 0x01 && (((received[1] << 8) | received[2]) == pckt.messageId))
                {
                    uint16_t receivedID = (received[1] << 8) | received[2];
                    receiveMap[receivedID] = true;
                    size_t messageContentsStart = 6; // Start after null terminator of DisplayName
                    std::string messageContents(received.begin() + messageContentsStart, received.end() - 1);
                    std::cerr << "Success: " << messageContents << std::endl;
                    return open;
                }
                else if (received[0] == 0x01 && received[3] == 0x00 && (((received[1] << 8) | received[2]) == pckt.messageId))
                {
                    uint16_t receivedID = (received[1] << 8) | received[2];
                    receiveMap[receivedID] = true;
                    size_t messageContentsStart = 6; // Start after null terminator of DisplayName
                    std::string messageContents(received.begin() + messageContentsStart, received.end() - 1);
                    std::cerr << "Failure: " << messageContents << std::endl;
                    if (authBool)
                    {
                        return auth;
                    }
                    return open;
                }
                // if not reply handle receive message
                States handle = handleRMessage(received);
                if (handle != open)
                {
                    return handle;
                }
            }
        }
    }
    std::cerr << "ERR: msg send failed after " << retransmissions << " tries." << std::endl;
    return end;
}

States udp::authMessage()
{
    // regex patterns for authorization
    std::regex pattern("[A-Za-z0-9-]{1,20}");
    std::regex secPattern("[A-Za-z0-9-]{1,128}");
    std::regex dNamePattern("[\x21-\x7E]{1,20}");

    std::string command;
    std::string username;
    std::string secret;
    std::string dnameI;
    std::string inputLine;
    // Construct the AUTH message
    uint8_t message_type = 0x02; // AUTH
                                 // You should assign a unique message ID
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
        }
        // save display name to class udp
        dname = dnameI;
        std::string word;
        if (!(iss >> word) && command == "/auth")
        {
            // create udp packet to store sended message
            UDPpckt pckt;
            size_t message_length = 1 + 2 + username.length() + 1 + dname.length() + 1 + secret.length() + 1;
            std::vector<uint8_t> message_buffer(message_length);
            size_t offset = 0;

            message_buffer[offset++] = message_type;
            message_buffer[offset++] = messageId >> 8;
            message_buffer[offset++] = messageId & 0xFF;

            memcpy(&message_buffer[offset], username.c_str(), username.length());
            offset += username.length();
            message_buffer[offset++] = 0;
            memcpy(&message_buffer[offset], dname.c_str(), dname.length());
            offset += dname.length();
            message_buffer[offset++] = 0;
            memcpy(&message_buffer[offset], secret.c_str(), secret.length());
            offset += secret.length();
            message_buffer[offset++] = 0;

            pckt.message_buffer = message_buffer;
            pckt.messageId = messageId;
            messageId++;

            return replyM(pckt, true);
        }
        else
        {
            std::cerr << "ERR: Expected exactly three words after /auth" << std::endl;
            return auth;
        }
    }

    return States::end;
}

States udp::handleRMessage(std::vector<char> received)
{
    // switch through message types
    switch (received[0])
    {
    case 0x00: // confirm
    {
        // check refID and delete packet from vector, to not send it again
        uint16_t refMessageID = (received[1] << 8) | received[2];
        auto it = std::find_if(pckts.begin(), pckts.end(), [&](const UDPpckt &p)
                               { return p.messageId == refMessageID; });
        if (it != pckts.end())
        {
            pckts.erase(it);
        }
        return open;
    }
    case 0x01: // reply
    {
        // if reply comes second time. nothing happens
        UDPpckt pckt;
        pckt.PCKTretransmissions = retransmissions;
        uint16_t receivedID = (received[1] << 8) | received[2];
        if (receiveMap.find(receivedID) != receiveMap.end())
        {
            return open;
        }

        // reply to nothing, message to server
        uint8_t message_type = 0xFE;
        std::string errorMessage = "reply to nothing";
        size_t message_length = 1 + 2 + dname.length() + 1 + errorMessage.length() + 1;
        std::vector<uint8_t> message_buffer(message_length);
        size_t offset = 0;

        message_buffer[offset++] = message_type;
        message_buffer[offset++] = messageId >> 8;
        message_buffer[offset++] = messageId & 0xFF;
        memcpy(&message_buffer[offset], dname.c_str(), dname.length());
        offset += dname.length();
        message_buffer[offset++] = 0;
        memcpy(&message_buffer[offset], errorMessage.c_str(), errorMessage.length());
        offset += errorMessage.length();
        message_buffer[offset++] = 0;

        pckt.message_buffer = message_buffer;
        pckt.messageId = messageId;
        messageId++;
        pckts.push_back(pckt);
        std::cerr << "ERR: got reply to nithing" << std::endl;

        if (!sendMessage(pckt))
        {
            return end;
        }

        return end;
    }
    case 0x04: // message
    {
        // if message received more than once nothing happend
        uint16_t receivedID = (received[1] << 8) | received[2];
        if (receiveMap.find(receivedID) != receiveMap.end())
        {
            return open;
        }
        // if received for first time, ID add to map
        receiveMap[receivedID] = true;
        // Extract DisplayName
        size_t displayNameStart = 3; // Start after message type and MessageID
        size_t displayNameEnd = displayNameStart;
        while (received[displayNameEnd] != 0x00 && displayNameEnd < received.size())
        {
            displayNameEnd++;
        }
        std::string displayName(received.begin() + displayNameStart, received.begin() + displayNameEnd);
        size_t messageContentsStart = displayNameEnd + 1;                                         // Start after null terminator of DisplayName
        std::string messageContents(received.begin() + messageContentsStart, received.end() - 1); // Exclude trailing null
        std::cout << displayName << ": " << messageContents << std::endl;
        return open;
    }
    case '\xfe': // error
    {
        size_t displayNameStart = 3; // Start after message type and MessageID
        size_t displayNameEnd = displayNameStart;
        while (received[displayNameEnd] != 0x00 && displayNameEnd < received.size())
        {
            displayNameEnd++;
        }
        std::string displayName(received.begin() + displayNameStart, received.begin() + displayNameEnd);
        size_t messageContentsStart = displayNameEnd + 1;                                         // Start after null terminator of DisplayName
        std::string messageContents(received.begin() + messageContentsStart, received.end() - 1); // Exclude trailing null
        std::cerr << "ERR FROM " << displayName << ": " << messageContents << std::endl;

        return end;
    }
    case '\xff': // BYE
        return end;

    default: // wrong received message
    {

        UDPpckt pckt;
        pckt.PCKTretransmissions = retransmissions;
        // sendign error to server
        uint8_t message_type = 0xFE;
        std::string errorMessage = "unreadable message";
        size_t message_length = 1 + 2 + dname.length() + 1 + errorMessage.length() + 1;
        std::vector<uint8_t> message_buffer(message_length);
        size_t offset = 0;

        message_buffer[offset++] = message_type;
        message_buffer[offset++] = messageId >> 8;
        message_buffer[offset++] = messageId & 0xFF;
        memcpy(&message_buffer[offset], dname.c_str(), dname.length());
        offset += dname.length();
        message_buffer[offset++] = 0;
        memcpy(&message_buffer[offset], errorMessage.c_str(), errorMessage.length());
        offset += errorMessage.length();
        message_buffer[offset++] = 0;

        pckt.message_buffer = message_buffer;
        pckt.messageId = messageId;
        messageId++;
        pckts.push_back(pckt);
        std::cerr << "ERR: unreadable message" << strerror(errno) << std::endl;
        if (!sendMessage(pckt))
        {
            return end;
        }

        return end;
    }
    }
    return end;
}

// check vector<UDPpckts> for timeout
bool udp::checkRetransmissions()
{
    auto currentTime = std::chrono::steady_clock::now();
    for (auto &pckt : pckts)
    {
        // time that elapsed from sending packet
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - pckt.sentTime);
        if (elapsed >= timeoutDuration)
        {
            // Resend packet
            if (!sendMessage(pckt))
            {
                return false;
            }
        }
    }
    return true;
}

States udp::openM()
{
    // regex check for rename and join commands
    std::regex pattern("[A-Za-z0-9-]{1,20}");
    std::regex dNamePattern("[\x21-\x7E]{1,20}");

    timeoutDuration = std::chrono::milliseconds(timeOut);

    // polls for receive and send
    struct pollfd fds[2];
    fds[0].fd = client_socket;
    fds[0].events = POLLIN;
    fds[1].fd = STDIN_FILENO;
    fds[1].events = POLLIN;

    while (true)
    {
        // check if timeout ocured
        if (!checkRetransmissions())
        {
            return end;
        }

        int ret = poll(fds, 2, 100);
        if (ret == -1)
        {
            std::cerr << "Poll error: " << strerror(errno) << std::endl;
            return end;
        }

        if (fds[0].revents & POLLIN)
        {

            auto recieved = receiveMessage();
            if (!recieved.empty())
            {
                States handle = handleRMessage(recieved);
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
            if (inputLine.substr(0, 7) == "/rename")
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
                    std::cerr << "ERR: wrong rename message." << std::endl;
                }
            }
            else if (inputLine.substr(0, 5) == "/join")
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
                        std::string channel = inputLine.substr(6, inputLine.length());

                        uint8_t message_type = 0x03;
                        UDPpckt pckt;
                        pckt.PCKTretransmissions = retransmissions;

                        size_t message_length = 1 + 2 + channel.length() + 1 + dname.length() + 1;
                        std::vector<uint8_t> message_buffer(message_length);
                        size_t offset = 0;

                        message_buffer[offset++] = message_type;
                        message_buffer[offset++] = messageId >> 8;
                        message_buffer[offset++] = messageId & 0xFF;
                        memcpy(&message_buffer[offset], channel.c_str(), channel.length());
                        offset += channel.length();
                        message_buffer[offset++] = 0;
                        memcpy(&message_buffer[offset], dname.c_str(), dname.length());
                        offset += dname.length();
                        message_buffer[offset++] = 0;

                        pckt.message_buffer = message_buffer;
                        pckt.messageId = messageId;
                        messageId++;
                        if (!checkRetransmissions())
                        {
                            return end;
                        }
                        pckts.push_back(pckt);
                        States handleJoin = replyM(pckts.back(), false);
                        if (handleJoin != open)
                        {
                            return handleJoin;
                        }
                    }
                }
                else
                {
                    std::cerr << "ERR: wrong join message." << std::endl;
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
            else // send MSG
            {
                uint8_t message_type = 0x04;
                UDPpckt pckt;
                pckt.PCKTretransmissions = retransmissions;

                size_t message_length = 1 + 2 + dname.length() + 1 + inputLine.length() + 1;
                std::vector<uint8_t> message_buffer(message_length);
                size_t offset = 0;

                message_buffer[offset++] = message_type;
                message_buffer[offset++] = messageId >> 8;
                message_buffer[offset++] = messageId & 0xFF;
                memcpy(&message_buffer[offset], dname.c_str(), dname.length());
                offset += dname.length();
                message_buffer[offset++] = 0;
                memcpy(&message_buffer[offset], inputLine.c_str(), inputLine.length());
                offset += inputLine.length();
                message_buffer[offset++] = 0;

                pckt.message_buffer = message_buffer;
                pckt.messageId = messageId;
                messageId++;
                pckts.push_back(pckt);
                if (!sendMessage(pckts.back()))
                {
                    return end;
                }
            }
        }
    }
    return end;
}
