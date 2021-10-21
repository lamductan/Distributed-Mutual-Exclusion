#include <dirent.h>
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
#include <sstream>
#include <thread>
#include <mutex>
#include <set>

#include "distributed-me/utils.h"
#include "distributed-me/node/server.h"
#include "distributed-me/CriticalSectionAlg/RACriticalSectionAlgStrategy.h"

Server::Server() {}

Server::Server(
        int id, const char* ip_addr, int port, int n_servers, int n_clients,
        const char* logdir, const char* group_server_hosted_directory) : 
        Node(id, ip_addr, port, n_servers, n_clients, logdir) {
    snprintf(
        hosted_directory, sizeof(hosted_directory), "%s/server_%d",
        group_server_hosted_directory, id);
    memset(client_done_all_request, false, this->n_clients);
}

NodeType Server::getNodeType() { return SERVER_NODE_TYPE; }

void Server::initLogger() {
    snprintf(logfile, sizeof(logfile), "%s/server_%d.txt", logdir, id);
    logger.set(logfile);
    snprintf(logfile2, sizeof(logfile2), "%s/server.txt", logdir);
    logger2.set(logfile2, "a");
}

const char* Server::getHostedDirectory() const {
    return hosted_directory;
}

void Server::serveClients(int max_clients) {
    printf(
        "Server %d is serving clients (maximum %d clients)\n", 
        id, max_clients);
    int new_client_socket;
    std::set<int> set_clients;

    // Wait for clients' connection. When a client comes, create a thread to
    // listen to their requests and handle them
    std::vector<std::thread> v_threads;
    char rbuff[MAX_BUFFER_SIZE];
    while (1) {
        new_client_socket = acceptNewSocket();
        int client_id;

        int valread = read(new_client_socket, rbuff, MAX_BUFFER_SIZE);
        if (valread > 0) {
            std::vector<Packet> vPackets = splitReadDataFromSocket(
                rbuff, valread);
            for(Packet& packet : vPackets) {
                client_id = packet.getSenderId();
                set_clients.insert(client_id);
                client_sockfd[client_id] = new_client_socket;
                printf(
                    "Server %d, new client_id = %d, new client socket = %d\n",
                    id, client_id, new_client_socket);
                logger.write(
                    "Server %d, new client_id = %d, new client socket = %d\n",
                    id, client_id, new_client_socket);
                if (new_client_socket < 0) {
                    printf("Server %d can't accept client\n", id);
                    break;
                }

                std::thread t = createThreadToHandleClientMessages(client_id);
                v_threads.push_back(std::move(t));
                usleep(100);
            }
        }
        if (set_clients.size() == n_clients) {
            break;
        } else {
            printf("set_clients.size() = %d\n", (int) set_clients.size());
        }
        sleep(1);
    }
    check(new_client_socket >= 0, "accept client failed");
    for(int i = 0; i < v_threads.size(); ++i) {
        v_threads[i].join();
    }
    for(int i = 0; i < n_clients; ++i) {
        close(client_sockfd[i]);
    }
    close(sockfd);
    printf("Server %d All done\n", id);
}

std::thread Server::createThreadToHandleClientMessages(
        int client_id) {
    int client_sock_fd = client_sockfd[client_id];
    printf(
        "Creating thread at server %d to handle messages from client %d at "
        "socket %d\n", id, client_id, client_sock_fd);
    logger.write(
        "Creating thread at server %d to handle messages from client %d at "
        "socket %d\n", id, client_id, client_sock_fd);
    std::pair<Server*, int>* p_args = new std::pair<Server*, int>(this, client_id);
    std::thread t(internalThreadForClientMessages, (void*) p_args);
    return std::move(t);
}

void* Server::internalThreadForClientMessages(void* p_args) {
    std::pair<Server*, int>* args_ = (std::pair<Server*, int>*) p_args;
    Server* p_server = args_->first;
    int client_id = args_->second;
    printf("*Server %d: client_id = %d, function: "
    "internalThreadForClientMessages\n", p_server->getId(), client_id);
    delete args_;
    p_server->handleClientMessages(client_id);
}

void Server::waitForAllClientsConnected() {
    while (true) {
        bool all_clients_connected = true;
        for(int i = 0; i < n_clients; ++i) {
            if (client_sockfd[i] == 0) {
                all_clients_connected = false;
                break;
            }
        }
        if (all_clients_connected) break;
        usleep(100);
    }
}

bool Server::checkAllClientsDoneAllRequest() {
    for(int i = 0; i < n_clients; ++i) {
        if (!client_done_all_request[i]) {
            return false;
        }
    }
    return true;
}

void Server::handleClientMessages(int client_id) {
    int client_sock_fd = client_sockfd[client_id];
    printf(
        "\tServer %d is handling messages from Client %d at sockfd = %d\n",
        id, client_id, client_sock_fd);
    logger.write(
        "This thread is for Server %d to handle messages from Client %d at "
        "socket with sockfd = %d\n", id, client_id, client_sock_fd);

    usleep(200);
    sendMessageToClient(
        client_id, SERVER_ECHO, "Hello from Server " + std::to_string(id));
    printf("Server %d sent greeting message to Client %d\n", id, client_id);

    char rbuff[MAX_BUFFER_SIZE];
    int max_times = MAX_SERVING_TIME;

    waitForAllClientsConnected();

    usleep(5000);
    while(true) {
        int valread = read(client_sock_fd, rbuff, MAX_BUFFER_SIZE);
        if (valread == -1) {
            continue;
        }

        printf("---Server %d, Client %d, sock_fd %d, valread = %d\n", id, client_id, client_sock_fd, valread);
        if (valread == 0) {
            break;
        } else {
            std::vector<Packet> vPackets = splitReadDataFromSocket(rbuff, valread);
            bzero(rbuff, MAX_BUFFER_SIZE);
            for(Packet& packet : vPackets) {
                MessageType message_type = packet.getMessageType();
                logger.write("Received packet = ");
                logger.writePacket(packet);
                if (message_type == CLIENT_ECHO) {
                    std::string msg = packet.getMsg();
                    printf("\tMessage from Client %d: %s\n",
                    client_id, msg.c_str());
                    logger.write("\tMessage from Client %d: %s\n",
                    client_id, msg.c_str());
                } else if (message_type == CLIENT_DONE_ALL_REQUEST) {
                    client_done_all_request[client_id] = true;
                } else if (message_type == CLIENT_ENQUIRY) {
                    // Response ENQUIRY request
                    handleEnquiryRequest(packet);
                } else if (message_type == CLIENT_REQUEST_READ) {
                    // Response READ request
                    handleReadRequest(packet);
                } else if (message_type == CLIENT_REQUEST_WRITE) {
                    // Response WRITE request
                    handleWriteRequest(packet);
                } else {
                    printf("Server %d: Request type %d is not supported\n",
                    id, message_type);
                    logger.write("Server %d: Request type %d is not supported\n",
                    id, message_type);
                }
                logger.write("===================================================\n");
            }
            usleep(100);
        }

        if (checkAllClientsDoneAllRequest()) {
            logger.write("All clients done all requests. SERVER_RECEIVED_ALL_DONE_REQUESTS are sent.\n");
            for(int i = 0; i < n_clients; ++i) {
                sendMessageToClient(i, SERVER_RECEIVED_ALL_DONE_REQUESTS, "");
            }
        }
        usleep(100);
    }
    printf("******Server %d thread to serve Client %d ends.\n", id, client_id);
    close(client_sock_fd);
}

void Server::handleEnquiryRequest(const Packet& packet) {
    g_mutex.lock();
    int client_id = packet.getSenderId();
    int client_sock_fd = client_sockfd[client_id];
    printf("Server %d is handling ENQUIRY request from Client %d\n",
    id, packet.getSenderId());
    logger.write("Server %d is handling ENQUIRY request from Client %d\n",
    id, packet.getSenderId());
    std::vector<std::string> list_hosted_files = getListHostedFiles();
    std::string response = "";
    int n_files = list_hosted_files.size();
    for(int i = 0; i < n_files; ++i) {
        response += list_hosted_files[i];
        if (i < n_files - 1) response += "#";
    }
    sendMessageToClient(client_id, SERVER_REPLY_ENQUIRY_CLIENT, response);
    
    printf("Response ENQUIRY request from Client %d: %s\n",
    client_id, response.c_str());
    logger.write("Response ENQUIRY request from Client %d: %s\n",
    client_id, response.c_str());
    g_mutex.unlock();
}

void Server::handleReadRequest(const Packet& packet) {
    g_mutex.lock();
    int client_id = packet.getSenderId();
    std::string filename = packet.getMsg();
    printf(
        "Server %d is handling READ request from Client %d, filename = %s\n",
        id, client_id, filename.c_str());
    logger.write(
        "Server %d is handling READ request from Client %d, filename = %s\n",
        id, client_id, filename.c_str());
    std::string last_line_of_file = readLastLineFile(filename);
    logger.write(
        "Server %d handled READ request from Client %d, filename = %s, "
        "response = %s\n", id, client_id, filename.c_str(),
        last_line_of_file.c_str());
    sendMessageToClient(
        client_id, SERVER_REPLY_READ_CLIENT, last_line_of_file);
    g_mutex.unlock();
}

void Server::handleWriteRequest(const Packet& packet) {
    g_mutex.lock();
    int client_id = packet.getSenderId();
    printf(
        "Server %d is handling WRITE request from Client %d\n", id, client_id);
    logger.write(
        "Server %d is handling WRITE request from Client %d\n", id, client_id);
    std::string msg = packet.getMsg();
    std::vector<std::string> tokens = splitString(msg, '#');
    std::string filename = tokens[0];
    std::string contentToWrite = tokens[1];
    logger.write("Content to write is: %s\n", contentToWrite.c_str());
    writeToFile(filename, contentToWrite);
    char sbuff[MAX_BUFFER_SIZE];
    snprintf(
        sbuff, MAX_BUFFER_SIZE, 
        "Server %d done writing line [%s] to file %s\n",
        id, contentToWrite.c_str(), filename.c_str());
    logger.write(sbuff);
    sendMessageToClient(
        client_id, SERVER_REPLY_WRITE_CLIENT, std::string(sbuff));
    g_mutex.unlock();
}

std::vector<std::string> Server::getListHostedFiles() {
    return getListFiles(hosted_directory);
}

std::string Server::getFilePath(const std::string& filename) {
    std::string hosted_directory_str = std::string(hosted_directory);
    return hosted_directory_str + "/" + filename;
}

std::string Server::readLastLineFile(const std::string& filename) {
    std::string filepath = getFilePath(filename);
    return _readLastLineOfFile(filepath);
}

void Server::writeToFile(
        const std::string& filename, const std::string& contentToWrite) {
    std::string filepath = getFilePath(filename);
    _writeToFile(filepath, contentToWrite);
}

void Server::run(int num_clients) {
    // Should call start() before run()
    serveClients(num_clients);
}

Server::~Server() {}