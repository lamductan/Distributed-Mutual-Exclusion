#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <thread>

#include "distributed-me/config.h"
#include "distributed-me/utils.h"
#include "distributed-me/node/client.h"

#define MAX_REQUESTS 10

int main(int argc, char** argv) {
    int client_id = atoi(argv[1]);
    const char* server_config_file = argv[2];
    const char* client_config_file = argv[3];
    int n_servers = atoi(argv[4]);
    int n_clients = atoi(argv[5]);
    const char* client_logdir = argv[6];
    const char* group_client_input_directory = argv[7];
    std::string alg_type = std::string(argv[8]);
    int n_requests = std::min(MAX_REQUESTS, atoi(argv[9]));
    int gen_random_input = atoi(argv[10]);

    CriticalSectionAlgType criticalSectionAlgType;
    if (alg_type == "RA")
        criticalSectionAlgType = RA_CRITICAL_SECTION_ALG;
    else if (alg_type == "MKW")
        criticalSectionAlgType = MKW_CRITICAL_SECTION_ALG;
    else criticalSectionAlgType = LAMPORT_CRITICAL_SECTION_ALG;

    std::vector<std::string> server_ip_addr_s;
    std::vector<int> server_port_s;
    getIpAddrsAndPortsFromConfig(server_config_file, server_ip_addr_s, server_port_s);

    std::vector<std::string> client_ip_addr_s;
    std::vector<int> client_port_s;
    getIpAddrsAndPortsFromConfig(client_config_file, client_ip_addr_s, client_port_s);

    usleep(5000);
    
    Client* client = new Client(
        client_id, client_ip_addr_s[client_id].c_str(),
        client_port_s[client_id], n_servers, n_clients, client_logdir,
        group_client_input_directory, criticalSectionAlgType,
        client_config_file);

    client->start();
    printf("Client %d starts\n", client->getId());

    usleep(5000);

    client->run(
        server_ip_addr_s, server_port_s, client_ip_addr_s, client_port_s,
        n_requests, gen_random_input);

    delete client;
    return 0;
}