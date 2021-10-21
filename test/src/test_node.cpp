#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <gtest/gtest.h>

#include "distributed-me/config.h"
#include "distributed-me/utils.h"
#include "distributed-me/node/node.h"
#include "distributed-me/node/server.h"
#include "distributed-me/node/client.h"
#include "distributed-me/groupserver.h"

struct TestNode
    : public ::testing::Test
{
    static Server* server;
    static Client* client;

    static void SetUpTestSuite() {
        server = new Server(3, "127.0.0.1", 12345, "../test_data/server/server_3", SERVER_LOG_DIR, RA_CRITICAL_SECTION_ALG);
        client = new Client();
    }

    static void TearDownTestSuite() {
        delete server;
        delete client;
    }
};

Server* TestNode::server = NULL;
Client* TestNode::client = NULL;

TEST_F(TestNode, TestSimpleServerClientConfig) {
    EXPECT_EQ(server->getNodeType(), SERVER_TYPE);
    EXPECT_EQ(client->getNodeType(), CLIENT_TYPE);
}

TEST_F(TestNode, TestSimpleServerWrite) {
    std::string filename = "test.txt";
    std::string content_to_write = "this is a unit test."; 
    server->writeToFile(filename, content_to_write);
    std::string lastline = server->readLastLineFile(filename);
    fprintf(stderr, "TestNode_TestSimpleServerWrite_lastline = %s\n", lastline.c_str());
    fprintf(stderr, "lastline = %s\n", lastline.c_str());
    EXPECT_EQ(content_to_write, lastline);
}

TEST_F(TestNode, TestGroupServer1) {
    std::vector<std::string> ip_addr_s;
    std::vector<int> port_s;
    getIpAddrsAndPortsFromConfig(CONFIG_FILE, ip_addr_s, port_s);

    for(int i = 0; i < ip_addr_s.size(); ++i) {
        fprintf(stderr, "***%s:%d\n", ip_addr_s[i].c_str(), port_s[i]);
    }

    GroupServer* group_server = new GroupServer(
        ip_addr_s, port_s, GROUP_SERVER_HOSTED_DIRECTORY, SERVER_LOG_DIR,
        RA_CRITICAL_SECTION_ALG);
    
    for(int i = 0; i < group_server->servers.size(); ++i) {
        Server* server = group_server->servers[i];
        fprintf(stderr, "hosted directory of server %d is %s\n", server->getId(), server->getHostedDirectory());
        std::vector<std::string> list_hosted_files = server->getListHostedFiles();
        fprintf(stderr, "List hosted files of server %d:\n", server->getId());
        for(std::string& hosted_file : list_hosted_files) {
            fprintf(stderr, "\t%s\n", hosted_file.c_str());
            std::string lastline = server->readLastLineFile(hosted_file);
            fprintf(stderr, "\tlastline = %s\n", lastline.c_str());
        }
    }

    delete group_server;
}