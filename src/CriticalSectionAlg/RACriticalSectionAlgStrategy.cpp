#include <unistd.h>
#include <stdarg.h>
#include <string>
#include <sstream>

#include "distributed-me/utils.h"
#include "distributed-me/node/client.h"
#include "distributed-me/CriticalSectionAlg/RACriticalSectionAlgStrategy.h"

RACriticalSectionAlgStrategy::RACriticalSectionAlgStrategy(Client* client, const std::string& filename) {
    ME = client->getId();
    n = client->getNClients();
    this->client = client;
    this->filename = filename;

    our_sequence_number = 0;
    hgst_sequence_number = 0;
    A.assign(n, false);
    is_using = false;
    is_waiting = false;
    reply_deferred.assign(n, false);
}

void RACriticalSectionAlgStrategy::set_n_clients(int n_clients) {
    CriticalSectionAlgStrategy::set_n_clients(n_clients);
    client->logger.write("RACAS[%s]: There are actually %d clients, n = %d\n", filename.c_str(), n_clients, n);
    A.assign(n, false);
    reply_deferred.assign(n, false);
    client->logger.write("RACAS[%s]: There are %d clients\n", filename.c_str(), n);
}

std::string RACriticalSectionAlgStrategy::get_msg() {
    std::string msg = "";
    msg += filename;
    msg += ';';
    msg += std::to_string(ME);
    msg += '#';
    msg += std::to_string(our_sequence_number);
    return msg;
}

// REQUEST message format: <filename>;<ME>#<our_sequence_number>
void RACriticalSectionAlgStrategy::request_resource() {
    g_mutex.lock();
    is_waiting = true;
    our_sequence_number = hgst_sequence_number + 1;
    client->logger.write("RACAS[%s]: I'm requesting resource, our_sequence_number = %d\n", filename.c_str(), our_sequence_number);
    std::vector<int> need_to_sends;
    for(int j = 0; j < n; ++j) {
        if (j != ME) {
            if (!A[j]) {
                client->logger.write("RACAS[%s]: I'm sending CLIENT_REQUEST_CS_CLIENT message to Client %d\n", filename.c_str(), j);
                need_to_sends.push_back(j);
            } else {
                client->logger.write("RACAS[%s]: I'm in need of critical section but I'm not sending CLIENT_REQUEST_CS_CLIENT message to Client %d because A[%d] = 1\n", filename.c_str(), j, j);
            }
        }
    }

    if (need_to_sends.size() > 0) {
        std::string msg = get_msg();
        for(int j : need_to_sends) {
            client->sendMessageToClient(j, CLIENT_REQUEST_CS_CLIENT, msg);
        }
    }
    g_mutex.unlock();

    while (!can_enter_critical_section()) {
        usleep(100);
    }
    enter_critical_section();
}

// REPLY message format: <filename>;<ME>#<our_sequence_number>
void RACriticalSectionAlgStrategy::release_resource() {
    g_mutex.lock();
    is_using = false;
    client->logger.write("**RACAS[%s]: I'm releasing resource\n", filename.c_str());
    std::vector<int> need_to_sends;
    for(int j = 0; j < n; ++j) {
        if (j == ME) continue;
        if (reply_deferred[j]) {
            client->logger.write("****RACAS[%s]: I'm releasing resource and sending CLIENT_REPLY_CS_CLIENT message to Client %d\n", filename.c_str(), j);
            A[j] = false;
            reply_deferred[j] = false;
            need_to_sends.push_back(j);
        } else {
            client->logger.write("****RACAS[%s]: I'm releasing resource but not sending CLIENT_REPLY_CS_CLIENT message to Client %d\n", filename.c_str(), j);
        }
    }
    if (need_to_sends.size() > 0) {
        std::string msg = get_msg();
        for(int j : need_to_sends) {
            client->sendMessageToClient(j, CLIENT_REPLY_CS_CLIENT, msg);
        }
    }
    g_mutex.unlock();
}

void RACriticalSectionAlgStrategy::treat_message(const Packet& packet) {
    MessageType message_type = packet.getMessageType();
    std::vector<std::string> tokens = splitString(packet.getMsg(), '#');
    int j = atoi(tokens[0].c_str());
    if (message_type == CLIENT_REPLY_CS_CLIENT) {
        treat_reply_message(j);
    } else if (message_type == CLIENT_REQUEST_CS_CLIENT) {
        int their_sequence_number = atoi(tokens[1].c_str());
        client->logger.write(
            "***RACAS[%s]: I'm treating REQUEST_CS message from Client %d, their_sequence_number = %d\n",
            filename.c_str(), j, their_sequence_number);
        treat_request_message(their_sequence_number, j);
    }
}

void RACriticalSectionAlgStrategy::treat_reply_message(int j) {
    g_mutex.lock();
    client->logger.write(
        "RACAS[%s]: I'm treating REPLY_CS message from Client %d\n", filename.c_str(), j);
    A[j] = true;
    g_mutex.unlock();
}

void RACriticalSectionAlgStrategy::treat_request_message(
        int their_sequence_number, int j) {
    client->logger.write("********RACAS[%s]:, function treat_request_message\n", filename.c_str());
    g_mutex.lock();
    client->logger.write(
        "RACAS[%s]: I'm treating REQUEST_CS message from Client %d -- ",
        filename.c_str(), j);
    
    hgst_sequence_number = std::max(
        hgst_sequence_number, their_sequence_number);
    bool our_priority = (their_sequence_number > our_sequence_number) 
        || (their_sequence_number == our_sequence_number && j > ME);
    
    if (is_using || (is_waiting && our_priority)) {
        reply_deferred[j] = true;
        client->logger.write("(is_using || (is_waiting && our_priority) --- ");
    }
    
    if (!(is_using || is_waiting) 
            || (is_waiting && !A[j] && !our_priority)) {
        A[j] = false;
        std::string msg = get_msg();
        client->sendMessageToClient(j, CLIENT_REPLY_CS_CLIENT, msg);
        
        client->logger.write(
            "(!(is_using || is_waiting) || (is_waiting && !A[j] && !our_priority)) --- ");
        client->logger.write("CLIENT_REPLY_CS_CLIENT is sent --- ");
    }
    
    if (is_waiting && A[j] && !our_priority) {
        A[j] = false;
        
        std::vector<int> need_to_sends(1, j);
        std::string msg = get_msg();
        client->sendMessageToClient(j, CLIENT_REPLY_CS_CLIENT, msg);
        
        client->logger.write(
            "RACAS[%s]: I'm sending REQUEST_CS message to Client %d ---- ",
            filename.c_str(), j);
        
        client->sendMessageToClient(j, CLIENT_REQUEST_CS_CLIENT, msg);
        
        client->logger.write("(is_waiting && A[j] && !our_priority) --- ");
        client->logger.write(
            "CLIENT_REPLY_CS_CLIENT is sent to Client %d --- ", j);
    }
    client->logger.write(
        "our_priority = %d, reply_deferred[%d] = %s, is_using = %s, is_waiting = %s, A[%d] = %s\n",
        our_priority, j, std::to_string(reply_deferred[j]).c_str(),
        std::to_string(is_using).c_str(), std::to_string(is_waiting).c_str(),
        j, std::to_string(A[j]).c_str());
    g_mutex.unlock();
}

bool RACriticalSectionAlgStrategy::can_enter_critical_section() {
    g_mutex.lock();
    bool all_A_is_true = true;
    std::vector<int> waiting_for;
    for(int j = 0; j < n; ++j) {
        if (j == ME) continue;
        if (!A[j]) waiting_for.push_back(j);
        all_A_is_true &= A[j];
    }
    //client->logger.write(
    //    "RACAS[%s]: Can I enter critical section? Answer is %s. I'm waiting for: [",
    //    filename.c_str(), (all_A_is_true ? "YES" : "NO"));
    //for(int x : waiting_for) client->logger.write("%d ", x);
    //client->logger.write("]\n");
    g_mutex.unlock();
    return all_A_is_true;
}

void RACriticalSectionAlgStrategy::enter_critical_section() {
    g_mutex.lock();
    client->logger.write("RACAS[%s]: I'm entering critical section\n", filename.c_str());
    is_waiting = false;
    is_using = true;
    g_mutex.unlock();
    client->enterCriticalSection();
    release_resource();
}

bool RACriticalSectionAlgStrategy::is_waiting_or_using_critical_section() {
    return (is_using || is_waiting);
}
