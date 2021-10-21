#include <stdio.h> 
#include <string.h>
#include <stdlib.h> 
#include <errno.h> 
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <sstream>

#include "distributed-me/utils.h"
#include "distributed-me/node/node.h"

Node::Node() {}

Node::Node(
		int id, const char* ip_addr, int port,
		int n_servers, int n_clients, const char* logdir) {
	this->id = id;
    strncpy(this->ip_addr, ip_addr, sizeof(this->ip_addr));
    this->port = port;
	this->n_servers = std::min(MAX_SERVERS,n_servers);
	this->n_clients = std::min(MAX_CLIENTS,n_clients);
    strncpy(this->logdir, logdir, sizeof(this->logdir));
	memset(server_sockfd, 0, this->n_servers);
    memset(client_sockfd, 0, this->n_clients);
}

int Node::getId() const { return id; }

int Node::getNServers() const { return n_servers; }

int Node::getNClients() const { return n_clients; }

int Node::getTimestamp() {
	return lamportClock.getTimestamp();
}

int Node::compare(const char* server_ip_addr, int server_port) {
    int cmp_ip_addr = strcmp(ip_addr, server_ip_addr);
    if (cmp_ip_addr != 0) return cmp_ip_addr;
    if (port == server_port) return 0;
    else if (port < server_port) return -1;
    else return 1;
}

void Node::start() {
	initLogger();
    startServerSocket();
}

void Node::startServerSocket() {
	printf("Starting server socket at %s:%d\n", ip_addr, port);
	
    //create a socket
	check((sockfd = socket(AF_INET, SOCK_STREAM, 0)), "socket failed");
	
    //type of socket created
    struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(ip_addr);
	address.sin_port = htons(port);
		
	//bind the socket
	check(bind(
        sockfd, (struct sockaddr *)&address, sizeof(address)) >= 0,
        "bind failed");
	logger.write("Listener at %s on port %d \n", ip_addr, port);
    check(listen(sockfd, MAX_CLIENTS) >= 0, "listen failed");
	logger2.write("Node %d starts.\n", id);
}

int Node::acceptNewSocket() {
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int new_socket = accept(
        sockfd,
        (struct sockaddr *)&address,
        (socklen_t*)&addrlen);
    check(new_socket >= 0, "failed to accept socket");

    printf(
		"New connection, socket fd is %d, ip is : %s, port : %d\n", new_socket,
		inet_ntoa(address.sin_addr), ntohs(address.sin_port));
    logger.write(
		"New connection, socket fd is %d, ip is : %s, port : %d\n", new_socket,
		inet_ntoa(address.sin_addr), ntohs(address.sin_port));
    return new_socket;
}

int Node::connectToServerSocket(const char* server_ip_addr, int server_port) {
	printf("Connecting to server at %s:%d\n", server_ip_addr, server_port);
	
	// socket create and verification 
	int sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	check(sockfd != -1, "socket creation failed...\n");
	printf("Socket successfully created..\n"); 
	
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr)); 

	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = inet_addr(server_ip_addr); 
	servaddr.sin_port = htons(server_port); 

	int n_tries = 0;
	
	while (true) {
		if (n_tries >= 10) {
			printf("connection with the server failed...\n");
			break;
		}
		if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) { 
			usleep(1000);
			n_tries++;
		} else {
			printf("connected to the server. sockfd = %d\n", sockfd);
			return sockfd;
		}
	}
	return -2;
}

Packet Node::createMessage(
		int receiver_id, MessageType message_type, const std::string& msg) {
	Packet sent_packet;
	sent_packet.setMessageType(message_type);
	sent_packet.setSenderId(this->id);
	sent_packet.setReceiverId(receiver_id);
	int timestamp = lamportClock.increaseTimestamp();
	sent_packet.setTimestamp(timestamp);
	sent_packet.setMsg(msg);
	return sent_packet;
}

void Node::sendMessageTo(
        NodeType receiver_node_type, int receiver_id, int receiver_sockfd,
		MessageType message_type, const std::string& msg) {
	Packet sent_packet = createMessage(receiver_id, message_type, msg);
	if (receiver_id == id && receiver_node_type == getNodeType()) {
		sendMessageToSelf(sent_packet);
	} else {
		char buffer[MAX_BUFFER_SIZE];
		sent_packet.toMessage(buffer, MAX_BUFFER_SIZE);
		send(receiver_sockfd, buffer, sent_packet.getMessageLength(), 0);
	}
}

void Node::sendMessageToServer(
		int server_id, MessageType message_type, const std::string& msg) {
	int server_sock_fd = server_sockfd[server_id];
	sendMessageTo(
		SERVER_NODE_TYPE, server_id, server_sock_fd, message_type, msg);
}

void Node::sendMessageToClient(
        int client_id, MessageType message_type, const std::string& msg) {
    int client_sock_fd = client_sockfd[client_id];
	sendMessageTo(
		CLIENT_NODE_TYPE, client_id, client_sock_fd, message_type, msg);
}

void Node::sendMessageToSelf(const Packet& packet) {
	message_queue.push(packet);
}

void Node::sendCriticalSectionMessageToClient(
        int client_id, MessageType message_type, 
        const std::string& msg, int our_sequence_number) {
	Packet sent_packet = createMessage(client_id, message_type, msg);
	sent_packet.setTimestamp(our_sequence_number);
	if (client_id == id && CLIENT_NODE_TYPE == getNodeType()) {
		sendMessageToSelf(sent_packet);
	} else {
		char buffer[MAX_BUFFER_SIZE];
		sent_packet.toMessage(buffer, MAX_BUFFER_SIZE);
		int receiver_sockfd = client_sockfd[client_id];
		send(receiver_sockfd, buffer, sent_packet.getMessageLength(), 0);
	}
}

std::string Node::toString() {
    std::stringstream ss;
    ss  << "{" << "id=" << id << "}";
    return ss.str();   
}

Node::~Node() {}