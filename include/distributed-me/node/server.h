#ifndef DISTRIBUTED_ME_SERVER_H_
#define DISTRIBUTED_ME_SERVER_H_

#include <stdio.h>
#include <netinet/in.h>
#include <vector>
#include <thread>
#include <string>
#include <queue>
#include <mutex>

#include "distributed-me/node/node.h"
#include "distributed-me/lamportclock.h"

class Server: public Node {
private:
    NodeType node_type = SERVER_NODE_TYPE;
    char hosted_directory[MAX_PATH_LENGTH - MAX_FILENAME_LENGTH - 4];
    std::vector<std::string> list_hosted_files;
    std::mutex g_mutex;
    bool client_done_all_request[MAX_CLIENTS];

private:
    void initLogger() override;
    void waitForAllClientsConnected();
    static void* internalThreadForClientMessages(void* p_args);
    bool checkAllClientsDoneAllRequest();
    std::string getFilePath(const std::string& filename);

public:
    Server();
    Server(
        int id, const char* ip_addr, int port, int n_servers, int n_clients,
        const char* logdir, const char* group_server_hosted_directory);
    NodeType getNodeType() override;
    const char* getHostedDirectory() const;

    void serveClients(int max_clients);
    void handleEnquiryRequest(const Packet& packet);
    void handleReadRequest(const Packet& packet);
    void handleWriteRequest(const Packet& packet);

    std::thread createThreadToHandleClientMessages(int client_id);
    void handleClientMessages(int client_id);

    std::vector<std::string> getListHostedFiles();
    std::string readLastLineFile(const std::string& filename);
    void writeToFile(
        const std::string& filename, const std::string& contentToWrite);

    void run(int num_clients);

    virtual ~Server();
};

#endif // DISTRIBUTED_ME_SERVER_H_