#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <thread>

#include "distributed-me/config.h"
#include "distributed-me/packet.h"
#include "distributed-me/utils.h"
#include "distributed-me/node/server.h"
#include "distributed-me/node/client.h"

#define MAX_TEST_SERVERS 3 // TODO: Change here
#define MAX_TEST_CLIENTS 5 // TODO: Change here

void test_packet_func() {
    Packet packet(CLIENT_ENQUIRY, 0, 1, 2, "lamductan");
    char message[1024];
    packet.toMessage(message, 1024);
    for(int i = 0; i < packet.getMessageLength(); ++i) {
        fprintf(stdout, "packet_to_message[%d] = %d\n", i, message[i]);
    }
}

void test1() {
    const char* client_config_file = "config_local/config_clients.txt";
    std::vector<std::string> client_ip_addr_s;
    std::vector<int> client_port_s;
    getIpAddrsAndPortsFromConfig(client_config_file, client_ip_addr_s, client_port_s);
    for(int i = 0; i < client_ip_addr_s.size(); ++i) {
        printf("%s - %d\n", client_ip_addr_s[i].c_str(), client_port_s[i]);
    }

    std::vector<std::vector<int>> quorums = getQuorumsFromConfig(
        client_config_file);
    for(int i = 0; i < quorums.size(); ++i) {
        printf("Quorum of Client %d: ", i);
        for(int q : quorums[i])
            printf("%d, ", q);
        printf("\n");
    }
}

int main() {
    PacketComparator packet_comparator;
    Packet packet_1(CLIENT_ENQUIRY, 0, 1, 1);
    Packet packet_2(CLIENT_ENQUIRY, 0, 1, 2, "lamductan");
    printf("%d\n", packet_comparator(packet_1, packet_2));
    return 0;
}
