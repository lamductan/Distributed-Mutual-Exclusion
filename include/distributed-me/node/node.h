#ifndef DISTRIBUTED_ME_NODE_H_
#define DISTRIBUTED_ME_NODE_H_

#include <string>

#include "distributed-me/config.h"
#include "distributed-me/packet.h"
#include "distributed-me/logger.h"
#include "distributed-me/lamportclock.h"
#include "distributed-me/messagequeue.h"

enum NodeType {
    SERVER_NODE_TYPE,
    CLIENT_NODE_TYPE
};

class Node {
protected:
    int id = 0;
    char ip_addr[16];
    int port;
    int sockfd;
    int n_servers;
    int n_clients;
    int server_sockfd[MAX_SERVERS];
    int client_sockfd[MAX_CLIENTS];
    LamportClock lamportClock;
    char logdir[MAX_PATH_LENGTH - MAX_FILENAME_LENGTH - 2];
    char logfile[MAX_PATH_LENGTH];
    char logfile2[MAX_PATH_LENGTH];
    Logger logger2;
    MessageQueue message_queue;

public:
    Logger logger;

protected:
    virtual void initLogger() = 0;
    int compare(const char* ip_addr, int port);
    void startServerSocket();
    Packet createMessage(
        int receiver_id, MessageType message_type, const std::string& msg);

public:
    Node();
    Node(
        int id, const char* ip_addr, int port,
        int n_servers, int n_clients, const char* logdir);
    virtual NodeType getNodeType() = 0;
    int getId() const;
    const char* getIpAddr() const;
    int getPort() const;
    int getNServers() const;
    int getNClients() const;
    int getTimestamp();
    void start();
    int acceptNewSocket();
    int connectToServerSocket(const char* server_ip_addr, int server_port);
    void sendMessageTo(
        NodeType receiver_node_type, int receiver_id, int receiver_sockfd,
		MessageType message_type, const std::string& msg);
    void sendMessageToServer(
        int server_id, MessageType message_type, const std::string& msg);
    void sendMessageToClient(
        int client_id, MessageType message_type, const std::string& msg);
    void sendCriticalSectionMessageToClient(
        int client_id, MessageType message_type, 
        const std::string& msg, int our_sequence_number);
    void sendMessageToSelf(const Packet& packet);
    std::string toString();
    virtual ~Node();
};

#endif // DISTRIBUTED_ME_NODE_H_