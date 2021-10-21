#ifndef DISTRIBUTED_ME_CLIENT_H_
#define DISTRIBUTED_ME_CLIENT_H_

#include <string>
#include <vector>
#include <map>
#include <thread>

#include "distributed-me/node/node.h"
#include "distributed-me/CriticalSectionAlg/CriticalSectionAlgStrategy.h"

class Client : public Node {
    enum RequestType {
        CLIENT_REQUEST_TYPE_READ,
        CLIENT_REQUEST_TYPE_WRITE
    };

    struct Request {
        RequestType request_type;
        std::string filename;
        int server_id;

        Request() {}

        Request(RequestType request_type, const std::string& filename) {
            this->request_type = request_type;
            this->filename = filename;
        }

        Request(RequestType request_type, const std::string& filename, int server_id) {
            this->request_type = request_type;
            this->filename = filename;
            this->server_id = server_id;
        }
    };

private:
    char client_input_file[MAX_PATH_LENGTH];
    CriticalSectionAlgType critical_section_alg_type;
    std::vector<std::string> list_files;
    bool critical_section_alg_init = false;
    std::map<std::string, CriticalSectionAlgStrategy*> critical_section_alg_strategy;
    char config_file_path[MAX_FILENAME_LENGTH];

    Request current_request;
    bool server_done_enquiry_request;
    bool server_done_read_request;
    bool server_done_write_request[MAX_SERVERS];
    bool request_all = false;
    int n_requested_requests = 0;
    int n_requests;
    bool all_clients_done_all_request = false;

private:
    void initLogger() override;
    void initCriticalSectionAlg();
    static void* internalThreadForServerMessages(void* p_args);
    static void* internalThreadForClientMessages(void* p_args);
    bool checkAllServersDoneWriteRequest();
    void handleClientPacket(Packet& packet);
public:
    Client();
    Client(
        int id, const char* ip_addr, int port, int n_servers, int n_clients,
        const char* logdir, const char* group_client_input_directory,
        CriticalSectionAlgType critical_section_alg_type,
        const char* config_file_path);
    NodeType getNodeType() override;
    std::vector<std::string> getListFiles();
    
    int connectToServer(int server_id, const char* server_ip_addr, int server_port);
    void connectToAllServers(
        const std::vector<std::string>& server_ip_addr_s,
        const std::vector<int>& server_port_s);
    int connectToOtherClient(int client_id, const char* client_ip_addr, int client_port);
    void connectToOtherClients(
        const std::vector<std::string>& client_ip_addr_s,
        const std::vector<int>& client_port_s);
    
    void testServerCommunicate(int server_id);
    void testServersCommunicate();
    void testClientCommunicate(int client_id);
    void testClientsCommunicate();

    std::vector<std::thread> createThreadToHandleServersMessages();
    std::thread createThreadToHandleServerMessages(int server_id);
    void handleServerMessages(int server_id);

    std::vector<std::thread> createThreadToHandleClientsMessages();
    std::thread createThreadToHandleClientMessages(int client_id);
    void handleClientMessages(int client_id);
    void handleSelfMessages();

    void generateRandomInputFile(const std::vector<std::string>& list_files, int n_requests);
    void requestServersFromInputFile(const char* input_file);
    void requestServersFromInputFile();

    void requestCriticalSection(const std::string& filename);
    void enterCriticalSection();

    void enquireListOfHostedFiles(int server_id);
    bool alreadyReceiveEnquireResult();
    std::vector<std::string> receiveEnquireResult(const Packet& packet);
    void printEnquiryResult(
        const std::vector<std::string>& enquiry_results, int server_id);
    void writeLogEnquiryResult(
        const std::vector<std::string>& enquiry_results, int server_id);

    void requestRead(const std::string& filename, int server_id);
    std::string receiveReadResult(const Packet& packet);
    void printReadResult(const std::string& read_result, int server_id);
    void writeLogReadResult(const std::string& read_result, int server_id);

    void requestWrite(const std::string& filename);
    std::string receiveWriteResult(const Packet& packet);
    void printWriteResult(const std::string& write_result, int server_id);
    void writeLogWriteResult(const std::string& write_result, int server_id);

    void run(
        std::vector<std::string> server_ip_addr_s,
        std::vector<int> server_port_s,
        std::vector<std::string> client_ip_addr_s,
        std::vector<int> client_port_s,
        int n_requests, int gen_random_input);

    virtual ~Client();
};

#endif // DISTRIBUTED_ME_CLIENT_H_