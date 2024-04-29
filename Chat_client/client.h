#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <string>
#include <sstream>
#include <cerrno>
#include <netinet/in.h>
#include <poll.h>
#include <thread>
#include <queue>
#include <csignal>
#include <vector>
#include <cstdint>
#include <chrono>
#include <map>
#include <regex>

// enumeration for FSM
enum States : int
{
    auth,
    open,
    end
};

// struct for flags
struct Parameters
{
    int tcpBool; // bool TCP or UDP
    const char *hostname = nullptr;
    uint16_t port;
    uint16_t timeout;
    uint8_t retransmissions;
};

// abstract class
class Protocol
{
protected:
    int client_socket;
    std::string dname;

public:
    virtual ~Protocol(){};
    virtual States authMessage() = 0;
    virtual States openM() = 0;
};

class tcp : public Protocol
{
public:
    tcp(Parameters params);
    ~tcp() override;
    States authMessage() override;
    States openM() override;
    std::vector<std::string> receiveMessage();
    void sendMessage(const std::string &message);
    States handleAuthMessage(std::vector<std::string> words);
    States sendJoin(const std::string &message);
    States handleRMessage(std::vector<std::string> words);
};

// struct for storing UDP packets
struct UDPpckt
{
    std::vector<uint8_t> message_buffer;
    uint16_t messageId;
    uint16_t PCKTretransmissions;
    // Use std::chrono for timestamps
    std::chrono::time_point<std::chrono::steady_clock> sentTime; // Store the time when packet was last sent
};
class udp : public Protocol
{
private:
    uint16_t messageId;
    struct sockaddr_in server_address;
    socklen_t server_address_size;
    std::vector<UDPpckt> pckts; // vector for storing UCP packets waitng to confirm
    uint16_t timeOut;
    uint16_t retransmissions;
    std::chrono::milliseconds timeoutDuration;
    std::map<uint16_t, bool> receiveMap; // if received messages was already handled it will be here and second time will do nothing

public:
    udp(Parameters params);
    ~udp() override;
    States authMessage() override;
    States openM() override;
    std::vector<char> receiveMessage();
    bool sendMessage(UDPpckt &pckt);
    States replyM(UDPpckt &pckt, bool authBool);
    bool checkRetransmissions();
    States handleRMessage(std::vector<char> received);
};
