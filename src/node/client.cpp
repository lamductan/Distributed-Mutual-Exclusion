#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include <errno.h> 
#include <assert.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <sys/time.h>
#include <string>
#include <thread>

#include "distributed-me/utils.h"
#include "distributed-me/node/client.h"
#include "distributed-me/CriticalSectionAlg/RACriticalSectionAlgStrategy.h"
#include "distributed-me/CriticalSectionAlg/MKWCriticalSectionAlgStrategy.h"

Client::Client() {}

Client::Client(
        int id, const char* ip_addr, int port, int n_servers, int n_clients,
        const char* logdir, const char* group_client_input_directory,
        CriticalSectionAlgType critical_section_alg_type,
        const char* config_file_path) :
        Node(id, ip_addr, port, n_servers, n_clients, logdir) {
    snprintf(
        this->client_input_file, sizeof(this->client_input_file), 
        "%s/client_%d.txt",group_client_input_directory, id);
    this->critical_section_alg_type = critical_section_alg_type;
    strncpy(
        this->config_file_path, config_file_path,
        sizeof(this->config_file_path));
}

void Client::initLogger() {
    snprintf(logfile, sizeof(logfile), "%s/client_%d.txt", logdir, id);
    logger.set(logfile);
    snprintf(logfile2, sizeof(logfile2), "%s/client.txt", logdir);
    logger2.set(logfile2, "a");
}

NodeType Client::getNodeType() { return CLIENT_NODE_TYPE; }

std::vector<std::string> Client::getListFiles() { return list_files; }

int Client::connectToServer(int server_id, 
        const char* server_ip_addr, int server_port) {
    printf(
        "Client %d connected to Server at ip %s and port %d\n",
        id, server_ip_addr, server_port);
    server_sockfd[server_id] = connectToServerSocket(
        server_ip_addr, server_port);
    sendMessageToServer(server_id, CLIENT_ECHO, "");
    //logger.write(
    //    "Client %d connected to Server at ip %s and port %d, at sock_fd = %d\n"
    //    , id, server_ip_addr, server_port, server_sockfd[server_id]);
    return server_sockfd[server_id];
}

void Client::connectToAllServers(
        const std::vector<std::string>& server_ip_addr_s,
        const std::vector<int>& server_port_s) {
    sleep(id + 1);
    for(int i = 0; i < n_servers; ++i) {
        connectToServer(i, server_ip_addr_s[i].c_str(), server_port_s[i]);
        usleep(100);
    }
}

int Client::connectToOtherClient(int client_id, 
        const char* client_ip_addr, int client_port) {
    int new_sockfd;
    int real_client_id;
    if (compare(client_ip_addr, client_port) < 0) {
        real_client_id = client_id;
        //actively connect to a greater client 
        printf("active from %d to %d\n", id, client_id);
        logger.write("active from %d to %d\n", id, client_id);
        new_sockfd = connectToServerSocket(client_ip_addr, client_port);
        client_sockfd[client_id] = new_sockfd;
        sendMessageToClient(client_id, CLIENT_ECHO, "");
    } else {
        //passively accept connection from smaller client
        new_sockfd = acceptNewSocket();
        char rbuff[MAX_BUFFER_SIZE];
        int valread = read(new_sockfd, rbuff, MAX_BUFFER_SIZE);
        Packet packet = MessageToPacket(rbuff);
        real_client_id = packet.getSenderId();
        client_sockfd[real_client_id] = new_sockfd;
        printf("passive from %d to %d\n", real_client_id, id);
        logger.write("passive from %d to %d\n", real_client_id, id);
    }
    printf("Client %d connected to Client %d\n", id, real_client_id);
    logger.write("Client %d connected to Client %d\n", id, real_client_id);
}

void Client::connectToOtherClients(
        const std::vector<std::string>& client_ip_addr_s,
        const std::vector<int>& client_port_s) {
    logger.write("n_clients = %d\n", n_clients);
    assert(n_clients <= MAX_CLIENTS);
    for(int i = 0; i < n_clients; ++i) {
        if (i == id) continue;
        connectToOtherClient(i, client_ip_addr_s[i].c_str(), client_port_s[i]);
        usleep(100);
	}

    printf("Client %d: client_socket = [", id);
    for(int i = 0; i < n_clients; ++i) {
        printf("%d ", client_sockfd[i]);
    }
    printf("]\n\n");

    logger.write("Client %d: client_socket = [", id);
    for(int i = 0; i < n_clients; ++i) {
        logger.write("%d ", client_sockfd[i]);
    }
    logger.write("]\n\n");
}

void Client::testServerCommunicate(int server_id) {
    usleep(1000);
    int server_sock_fd = server_sockfd[server_id];
    printf(
        "Testing Client %d communicates Server %d, server_sock_fd = %d\n",
        id, server_id, server_sock_fd);
    char rbuff[MAX_BUFFER_SIZE];
    
    int valread = read(server_sock_fd, rbuff, MAX_BUFFER_SIZE);
    if (valread == -1) {
        printf("Client %d timeout waiting for server %d\n", id, server_id);
        return;
    }
    else if (valread == 0) {
        printf("Server %d crashes\n", server_id);
    } else {
        //logger.write(
        //    "Client %d received greeting message from server %d\n",
        //    id, server_id);
        Packet packet = MessageToPacket(rbuff);
        //logger.write(
        //    "Greeting message from server %d, type = %d\n",
        //    server_id, packet.getMessageType());
        if (packet.getMessageType() == SERVER_ECHO) {
            logger.write("I'm gonna reply to server %d\n", server_id);
            std::string msg = "Hello this is a text from client " 
                + std::to_string(id) + " to server "
                + std::to_string(server_id);
            sendMessageToServer(server_id, CLIENT_ECHO, msg);
            printf(
                "Testing client %d communicates server %d: greeting message was"
                " sent\n", id, server_id);
        }
    }
}

void Client::testServersCommunicate() {
    printf("Testing client %d communicates with other servers\n", id);
    for(int i = 0; i < n_servers; ++i) {
        testServerCommunicate(i);
    }
}

void Client::testClientCommunicate(int client_id) {
    usleep(100);
    std::string msg = "Hello this is a text from Client " + std::to_string(id)
        + " to Client " + std::to_string(client_id);
    sendMessageToClient(client_id, CLIENT_ECHO, msg);
}

void Client::testClientsCommunicate() {
    printf("Testing Client %d communicates with other Clients\n", id);
    for(int i = 0; i < n_clients; ++i) {
        if (i == id) continue;
        testClientCommunicate(i);
    }
}

std::vector<std::thread> Client::createThreadToHandleServersMessages() {
    std::vector<std::thread> v_threads;
    for(int i = 0; i < n_servers; ++i) {
        std::thread t = createThreadToHandleServerMessages(i);
        v_threads.push_back(std::move(t));
    }
    return v_threads;
}

std::thread Client::createThreadToHandleServerMessages(int server_id) {
    int server_sock_fd = server_sockfd[server_id];
    printf(
        "Creating thread at Client %d to handle messages from Server %d at "
        "socket %d\n", id, server_id, server_sock_fd);
    //logger.write(
    //    "Creating thread at Client %d to handle messages from Server %d at "
    //    "socket %d\n", id, server_id, server_sock_fd);
    std::pair<Client*, int>* p_args = new std::pair<Client*, int>(this, server_id);
    std::thread t(internalThreadForServerMessages, (void*) p_args);
    return std::move(t);
}

void* Client::internalThreadForServerMessages(void* p_args) {
    std::pair<Client*, int>* args_ = (std::pair<Client*, int>*) p_args;
    Client* client = args_->first;
    int server_id = args_->second;
    printf(
        "*Client %d creates thread to handle Server %d's messages\n",
        client->getId(), server_id);
    delete args_;
    client->handleServerMessages(server_id);
}

bool isCriticalSectionAlgMessage(
        MessageType message_type, 
        CriticalSectionAlgType critical_section_alg_type) {
    if (critical_section_alg_type == RA_CRITICAL_SECTION_ALG) {
        switch (message_type) {
            case CLIENT_REQUEST_CS_CLIENT:
                return true;
            case CLIENT_REPLY_CS_CLIENT:
                return true;
            default:
                return false;
        }
    } else if (critical_section_alg_type == MKW_CRITICAL_SECTION_ALG) {
        switch (message_type) {
            case CLIENT_MKW_REQUEST_CLIENT:
                return true;
            case CLIENT_MKW_INQUIRE_CLIENT:
                return true;
            case CLIENT_MKW_RELINQUISH_CLIENT:
                return true;
            case CLIENT_MKW_FAILED_CLIENT:
                return true;
            case CLIENT_MKW_LOCKED_CLIENT:
                return true;
            case CLIENT_MKW_RELEASE_CLIENT:
                return true;
            default:
                return false;
        }
    }
}

void Client::handleServerMessages(int server_id) {
    int server_sock_fd = server_sockfd[server_id];
    printf(
        "Client %d is handling messages from Server %d at sockfd = %d\n",
        id, server_id, server_sock_fd);
    //logger.write(
    //    "This thread is for Client %d to handle messages from Server %d at "
    //    "socket with sockfd = %d\n", id, server_id, server_sock_fd);
    char rbuff[MAX_BUFFER_SIZE];
    
    while(true) {
        //logger.write(
        //    "\tRECEIVE THREAD from Server %d, waiting for a packet coming "
        //    "from Server %d\n", server_id, server_id);

        int valread = read(server_sock_fd, rbuff, MAX_BUFFER_SIZE);
        if (valread == -1) {
            continue;
        }
        if (valread == 0) {
            break;
        } else {
            std::vector<Packet> vPackets = splitReadDataFromSocket(rbuff, valread);
            bzero(rbuff, MAX_BUFFER_SIZE);
            for(Packet& packet : vPackets) {
                logger.write(
                    "Client %d received a message from Server %d, content of "
                    "the packet is:\n", id, packet.getSenderId());
                //logger.writePacket(packet);
                //logger.write("\n");
                MessageType message_type = packet.getMessageType();
                if (message_type == SERVER_ECHO) {
                    std::string msg = packet.getMsg();
                    printf("\tMessage from Server %d: %s\n",
                        server_id, msg.c_str());
                    logger.write("\tMessage from Server %d: %s\n",
                        server_id, msg.c_str());
                } else if (message_type == SERVER_RECEIVED_ALL_DONE_REQUESTS){
                    all_clients_done_all_request = true;
                    for(int i = 0; i < n_clients; ++i) {
                        sendMessageToClient(i, CLIENT_DONE_ALL_REQUEST, "");
                    }
                    break;
                } else if (message_type == SERVER_REPLY_ENQUIRY_CLIENT) {
                    list_files = receiveEnquireResult(packet);
                    printEnquiryResult(list_files, server_id);
                    writeLogEnquiryResult(list_files, server_id);
                } else if (message_type == SERVER_REPLY_READ_CLIENT) {
                    std::string read_result = receiveReadResult(packet);
                    printReadResult(read_result, server_id);
                    writeLogReadResult(read_result, server_id);
                } else if (message_type == SERVER_REPLY_WRITE_CLIENT) {
                    std::string write_result = receiveWriteResult(packet);
                    printWriteResult(write_result, server_id);
                    writeLogWriteResult(write_result, server_id);
                } else {
                    logger.write("Wrong message type\n");
                }
            }
            usleep(100);
        }
        if (all_clients_done_all_request) break;
    }
    printf(
        "******Client %d's thread to handle Server %d's messages ends.\n",
        id, server_id);
    logger.write(
        "******Client %d's thread to handle Server %d's messages ends.\n",
        id, server_id);
    server_sockfd[server_id] = 0;
    close(server_sock_fd);
}

std::vector<std::thread> Client::createThreadToHandleClientsMessages() {
    std::vector<std::thread> v_threads;
    for(int i = 0; i < n_clients; ++i) {
        std::thread t = createThreadToHandleClientMessages(i);
        v_threads.push_back(std::move(t));
    }
    return v_threads;
}

std::thread Client::createThreadToHandleClientMessages(int client_id) {
    int client_sock_fd = client_sockfd[client_id];
    printf(
        "Creating thread at Client %d to handle messages from Client %d at "
        "socket %d\n", id, client_id, client_sock_fd);
    //logger.write(
    //    "Creating thread at Client %d to handle messages from Client %d at "
    //    "socket %d\n", id, client_id, client_sock_fd);
    std::pair<Client*, int>* p_args = new std::pair<Client*, int>(this, client_id);
    std::thread t(internalThreadForClientMessages, (void*) p_args);
    return std::move(t);
}

void* Client::internalThreadForClientMessages(void* p_args) {
    std::pair<Client*, int>* args_ = (std::pair<Client*, int>*) p_args;
    Client* client = args_->first;
    int client_id = args_->second;
    printf(
        "*Client %d creates thread to handle Client %d's messages\n",
        client->getId(), client_id);
    delete args_;
    if (client->id == client_id) {
        client->handleSelfMessages();
    } else {
        client->handleClientMessages(client_id);
    }
}

void Client::handleClientMessages(int client_id) {
    int client_sock_fd = client_sockfd[client_id];
    printf(
        "Client %d is handling messages from Client %d at sockfd = %d\n",
        id, client_id, client_sock_fd);
    //logger.write(
    //    "This thread is for Client %d to handle messages from Client %d at "
    //    "socket with sockfd = %d\n", id, client_id, client_sock_fd);
    char rbuff[MAX_BUFFER_SIZE];

    while(true) {
        //logger.write(
        //    "\tRECEIVE THREAD from Client %d, waiting for a packet coming "
        //    "from Client %d\n", client_id, client_id);

        int valread = read(client_sock_fd, rbuff, MAX_BUFFER_SIZE);
        if (valread == -1) {
            continue;
        }
        if (valread == 0) {
            logger.write(
                "Client %d Thread to handle messages from Client %d at socket "
                "with sockfd = %d\n: valread = 0", id, client_id,
                client_sock_fd);
            break;
        } else {
            std::vector<Packet> vPackets = splitReadDataFromSocket(rbuff, valread);
            bzero(rbuff, MAX_BUFFER_SIZE);
            for(Packet& packet : vPackets) {
                if (packet.getSenderId() != client_id) {
                    logger.write("Wrong sockfd. Intend to receive message from Client %d, but get message from Client %d\n", client_id, packet.getSenderId());
                }
                handleClientPacket(packet);
                usleep(100);
            }
            usleep(100);
        }
        if (all_clients_done_all_request) break;
    }
    printf(
        "******Client %d's thread to handle Client %d's messages ends.\n",
        id, client_id);
    logger.write(
        "******Client %d's thread to handle Client %d's messages ends.\n",
        id, client_id);
    client_sockfd[client_id] = 0;
    close(client_sock_fd);
}

void Client::handleSelfMessages() {
    printf("Client %d is handling its self messages\n", id);
    logger.write("This thread is for Client %d to handle its self messages\n", id);
    while(true) {
        if (!message_queue.empty()) {
            Packet packet = message_queue.top();
            message_queue.pop();
            handleClientPacket(packet);
        }
        if (all_clients_done_all_request) break;
        usleep(100);
    }
    printf(
        "******Client %d's thread to handle its self messages ends.\n", id);
    logger.write(
        "******Client %d's thread to handle its self messages ends.\n", id);
}

void Client::handleClientPacket(Packet& packet) {
    int client_id = packet.getSenderId();
    logger.write(
        "Client %d received a message from Client %d, content of "
        "the packet is:\n", id, packet.getSenderId());
    //logger.writePacket(packet);
    //logger.write("\n");
    MessageType message_type = packet.getMessageType();
    if (message_type == CLIENT_ECHO) {
        std::string msg = packet.getMsg();
        printf("\tMessage from Client %d: %s\n",
            client_id, msg.c_str());
        logger.write("\tMessage from Client %d: %s\n",
            client_id, msg.c_str());
    } else if (message_type == CLIENT_DONE_ALL_REQUEST) {
        all_clients_done_all_request = true;
    } else if (isCriticalSectionAlgMessage(
            message_type, critical_section_alg_type)) { 
        // handle distributed mutual exclusion algorithm messages
        logger.write(
            "Receive a distributed mutual exclusion "
            "algorithm message: %d from Client %d\n",
            packet.getMessageType(), packet.getSenderId());
        std::vector<std::string> tokens = splitString(packet.getMsg(), ';');
        std::string filename = tokens[0];
        std::string msg = tokens[1];
        packet.setMsg(msg);
        critical_section_alg_strategy[filename]->treat_message(packet);
    }
}

void Client::generateRandomInputFile(
        const std::vector<std::string>& list_files, int n_requests) {
    _generateRandomInputFile(
        list_files, client_input_file, n_requests, n_servers, id);
}

void Client::requestServersFromInputFile(const char* input_file) {
    // Only send request, receive results in another thread
    printf("Client %d is doing request from input file %s\n", id, input_file);
    logger.write(
        "Client %d is doing request from input file %s\n", id, input_file);

    // Read input file line by line and perform request
    FILE* f = fopen(input_file, "r");
    if (f != NULL) {
        char request_type[10];
        char filename[MAX_FILENAME_LENGTH];
        std::string filename_str;
        int server_id;
        char buffer[MAX_BUFFER_SIZE];
        while(fgets(buffer, MAX_BUFFER_SIZE, f) != NULL) {
            if (strlen(buffer) <= 1) continue;
            sscanf(buffer, "%s", request_type);
            if (strcmp(request_type, "enquiry") == 0) {
                // Request enquiry
                sscanf(buffer, "%s %d", request_type, &server_id);
                printf("ENQUIRY %d\n", server_id);
                logger.write("ENQUIRY %d\n", server_id);
                enquireListOfHostedFiles(server_id);
                filename_str = "";
            } else if (strcmp(request_type, "read") == 0) {
                // Request read
                sscanf(buffer, "%s %s %d", request_type, filename, &server_id);
                printf("READ %s %d\n", filename, server_id);
                logger.write("READ %s %d\n", filename, server_id);
                filename_str = std::string(filename);
                current_request = Request(CLIENT_REQUEST_TYPE_READ, filename_str, server_id);
                requestCriticalSection(filename_str);
            } else if (strcmp(request_type, "write") == 0) {
                // Request write
                sscanf(buffer, "%s %s", request_type, filename);
                printf("WRITE %s\n", filename);
                logger.write("WRITE %s\n", filename);
                filename_str = std::string(filename);
                current_request = Request(CLIENT_REQUEST_TYPE_WRITE, filename_str);
                requestCriticalSection(filename_str);
            } else {
                printf("command %s not supported\n", request_type);
                logger.write("command %s not supported\n", request_type);
            }
            usleep(200);
        }
    }
    fclose(f);
    request_all = true;
    printf("Client %d requests all requests\n", id);
    logger.write("Client %d requests all requests. request_all = %d.\n", id, request_all);
}

void Client::requestServersFromInputFile() {
    requestServersFromInputFile(client_input_file);
}

// Enquiry
void Client::enquireListOfHostedFiles(int server_id) {
    server_done_enquiry_request = false;
    printf(
        "Client %d is sending ENQUIRY request to server %d\n", id, server_id);
    logger.write(
        "Client %d is sending ENQUIRY request to server %d\n", id, server_id);
    sendMessageToServer(server_id, CLIENT_ENQUIRY, "enquire request");
    while (!server_done_enquiry_request) {
        usleep(1000);
    }
}

bool Client::alreadyReceiveEnquireResult() {
    return server_done_enquiry_request;
}

std::vector<std::string> Client::receiveEnquireResult(const Packet& packet) {
    printf(
        "Message_type = %d, response = %s\n", packet.getMessageType(),
        packet.getMsg().c_str());
    logger.write(
        "Message_type = %d, response = %s\n", packet.getMessageType(),
        packet.getMsg().c_str());
    std::vector<std::string> enquiry_results;
    if (packet.getMessageType() == SERVER_REPLY_ENQUIRY_CLIENT) {
        std::string response = packet.getMsg();
        enquiry_results = splitString(response, '#');
    } else {
        printf(
            "Wrong reply type, expect SERVER_REPLY_ENQUIRY_CLIENT, "
            "receive %d\n", packet.getMessageType());
        logger.write(
            "Wrong reply type, expect SERVER_REPLY_ENQUIRY_CLIENT, "
            "receive %d\n", packet.getMessageType());
    }

    list_files = enquiry_results;
    initCriticalSectionAlg();
    server_done_enquiry_request = true;
    return enquiry_results;
}

void Client::printEnquiryResult(
        const std::vector<std::string>& enquiry_results, int server_id) {
    printf("\nClient %d ENQUIRY results from Server %d\n", id, server_id);
    for(int i = 0; i < enquiry_results.size(); ++i) {
        printf("%s\n", enquiry_results[i].c_str());
    }
    printf("\n");
}

void Client::writeLogEnquiryResult(
        const std::vector<std::string>& enquiry_results, int server_id) {
    logger.write(
        "\nClient %d ENQUIRY results from Server %d\n", id, server_id);
    for(int i = 0; i < enquiry_results.size(); ++i) {
        logger.write("%s\n", enquiry_results[i].c_str());
    }
    logger.write("\n");
}

void Client::initCriticalSectionAlg() {
    if (critical_section_alg_init == true)
        return;
    critical_section_alg_init = true;
    if (critical_section_alg_type == RA_CRITICAL_SECTION_ALG) {
        for(int i = 0; i < list_files.size(); ++i) {
            std::string filename = list_files[i];
            this->critical_section_alg_strategy[filename] = new RACriticalSectionAlgStrategy(this, filename);
        }
    } else if (critical_section_alg_type == MKW_CRITICAL_SECTION_ALG) {
        std::vector<std::vector<int>> quorums = getQuorumsFromConfig(
            config_file_path);
        std::vector<int> quorum = quorums[id];
        for(int i = 0; i < list_files.size(); ++i) {
            std::string filename = list_files[i];
            this->critical_section_alg_strategy[filename] 
            = new MKWCriticalSectionAlgStrategy(this, filename, quorum);
        }
    } else {
        printf("This critical section algorithm is currently not supported\n");
        exit(EXIT_FAILURE);
    }
}

void Client::requestCriticalSection(const std::string& filename) {
    ++n_requested_requests;
    critical_section_alg_strategy[filename]->request_resource();
}

void Client::enterCriticalSection() {
    printf("\n**Client %d is entering critical section\n", id);
    logger.write("\n**Client %d is entering critical section\n", id);
    if (current_request.request_type == CLIENT_REQUEST_TYPE_READ) {
        std::string filename = current_request.filename;
        int server_id = current_request.server_id;
        requestRead(filename, server_id);
    } else if (current_request.request_type == CLIENT_REQUEST_TYPE_WRITE) {
        std::string filename = current_request.filename;
        requestWrite(filename);
    } else {
        printf("Wrong request type\n");
        logger.write("Wrong request type\n");
    }
    printf("=== n_requested_requests = %d ===\n", n_requested_requests);
    logger.write("=== n_requested_requests = %d ===\n", n_requested_requests);
    if (n_requested_requests == n_requests) {
        logger.write("=====Client %d done all requests. request_all = %d =====\n", id, request_all);
        for(int i = 0; i < n_servers; ++i) {
            sendMessageToServer(i, CLIENT_DONE_ALL_REQUEST, "");
        }
    }
}

// Read
void Client::requestRead(const std::string& filename, int server_id) {
    server_done_read_request = false;
    printf("Client %d is sending READ request to server %d\n", id, server_id);
    logger.write(
        "Client %d is sending READ request to server %d\n", id, server_id);
    sendMessageToServer(server_id, CLIENT_REQUEST_READ, std::string(filename));
    while (!server_done_read_request) {
        logger.write("Waiting for READ result from Server %d\n", server_id);
        usleep(1000);
    }
}

std::string Client::receiveReadResult(const Packet& packet) {
    printf(
        "Message_type = %d, response = %s\n", packet.getMessageType(),
        packet.getMsg().c_str());
    logger.write(
        "Message_type = %d, response = %s\n", packet.getMessageType(),
        packet.getMsg().c_str());
    if (packet.getMessageType() == SERVER_REPLY_READ_CLIENT) {
        std::string response = packet.getMsg();
        server_done_read_request = true;
        return response;
    } else {
        printf(
            "Wrong reply type, expect SERVER_REPLY_READ_CLIENT, receive %d\n",
            packet.getMessageType());
        logger.write(
            "Wrong reply type, expect SERVER_REPLY_READ_CLIENT, receive %d\n",
            packet.getMessageType());
    }
    return "";
}

void Client::printReadResult(
        const std::string& read_result, int server_id) {
    printf("\nClient %d READ result from Server %d\n", id, server_id);
    printf("%s\n", read_result.c_str());
    printf("\n");
}

void Client::writeLogReadResult(
        const std::string& read_result, int server_id) {
    logger.write("\nClient %d READ result from Server %d\n", id, server_id);
    logger.write("%s\n", read_result.c_str());
    logger.write("\n");
}

// Write
void Client::requestWrite(const std::string& filename) {
    int timestamp = lamportClock.increaseTimestamp();
    std::string write_request = std::string(filename) 
        + "#" + std::to_string(id) + "," + std::to_string(timestamp);
    memset(server_done_write_request, false, n_servers);
    for(int i = 0; i < n_servers; ++i) {
        sendMessageToServer(i, CLIENT_REQUEST_WRITE, write_request);
    }

    while (!checkAllServersDoneWriteRequest()) {
        logger.write("Waiting for WRITE result\n");
        usleep(1000);
    }
}

bool Client::checkAllServersDoneWriteRequest() {
    for(int i = 0; i < n_servers; ++i) {
        if (!server_done_write_request[i]) return false;
    }
    return true;
}

std::string Client::receiveWriteResult(const Packet& packet) {
    if (packet.getMessageType() == SERVER_REPLY_WRITE_CLIENT) {
        int server_id = packet.getSenderId();
        server_done_write_request[server_id] = true;
        std::string msg = packet.getMsg();
        return msg;
    } else {
        printf(
            "Wrong reply type, expect SERVER_REPLY_WRITE_CLIENT, receive %d\n",
            packet.getMessageType());
        logger.write(
            "Wrong reply type, expect SERVER_REPLY_WRITE_CLIENT, receive %d\n",
            packet.getMessageType());
        return "";
    }
}

void Client::printWriteResult(
        const std::string& write_result, int server_id) {
    printf("%s\n", write_result.c_str());
}

void Client::writeLogWriteResult(
        const std::string& write_result, int server_id) {
    logger.write("%s\n", write_result.c_str());
}

void Client::run(
        std::vector<std::string> server_ip_addr_s,
        std::vector<int> server_port_s,
        std::vector<std::string> client_ip_addr_s,
        std::vector<int> client_port_s,
        int n_requests, int gen_random_input) {
    // Should call start() before run()
    this->n_requests = n_requests;
    while (countLine(logger2.getFilename()) < n_clients) {
        usleep(1000);
    }
    while (countLine(std::string(logdir) + "/server.txt") < n_servers) {
        usleep(1000);
    }

    connectToOtherClients(client_ip_addr_s, client_port_s);
    usleep(1000);
    testClientsCommunicate();

    connectToAllServers(server_ip_addr_s, server_port_s);
    usleep(1000);
    testServersCommunicate();

    usleep(1000);
    std::vector<std::thread> v_server_threads = createThreadToHandleServersMessages();

    usleep(1000);
    enquireListOfHostedFiles(rand() % getNServers());
    while (getListFiles().size() == 0) {
        usleep(100);
    }

    usleep(1000);
    std::vector<std::thread> v_client_threads = createThreadToHandleClientsMessages();

    if (n_requests > 0 && gen_random_input > 0) {
        generateRandomInputFile(getListFiles(), n_requests);
    }

    usleep(5000);
    requestServersFromInputFile();

    for(int i = 0; i < v_client_threads.size(); ++i) {
        v_client_threads[i].join();
    }

    for(int i = 0; i < v_server_threads.size(); ++i) {
        v_server_threads[i].join();
    }

    close(sockfd);
    printf("\nClient %d ends.\n", id);
    logger.write("\nClient %d ends.\n", id);
}

Client::~Client() {}