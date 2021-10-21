#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <thread>

#include "distributed-me/config.h"
#include "distributed-me/utils.h"
#include "distributed-me/node/server.h"

int main(int argc, char** argv) {
    int server_id = atoi(argv[1]);
    const char* config_file = argv[2];
    int n_servers = atoi(argv[3]);
    int n_clients = atoi(argv[4]);
    const char* server_logdir = argv[5];
    const char* group_server_hosted_directory = argv[6];

    std::vector<std::string> ip_addr_s;
    std::vector<int> port_s;
    getIpAddrsAndPortsFromConfig(config_file, ip_addr_s, port_s);
    
    Server* server = new Server(
        server_id, ip_addr_s[server_id].c_str(), port_s[server_id],
        n_servers, n_clients, server_logdir, group_server_hosted_directory);

    server->start();
    printf("Server %d starts\n", server->getId());

    usleep(5000);
    server->run(MAX_CLIENTS);

    printf("\n Server %d ends.\n", server->getId());
    delete server;
    return 0;
}