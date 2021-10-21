#include <unistd.h>
#include <stdarg.h>
#include <string>
#include <sstream>

#include "distributed-me/utils.h"
#include "distributed-me/node/client.h"
#include "distributed-me/CriticalSectionAlg/MKWCriticalSectionAlgStrategy.h"

MKWCriticalSectionAlgStrategy::MKWCriticalSectionAlgStrategy(
        Client* client, const std::string& filename,
        const std::vector<int>& quorum) {
    ME = client->getId();
    n = client->getNClients();
    this->client = client;
    this->filename = filename;
    this->quorum = quorum;

    our_sequence_number = 0;
    hgst_sequence_number = 0;

    is_inquire_replied.assign(n, true);

    client->logger.write("quorum: [");
    for(int i : quorum) {
        client->logger.write("%d ", i);
    }
    client->logger.write("]\n");
    reset_request_reply();
    locking_request.setSenderId(-1);
}

void MKWCriticalSectionAlgStrategy::reset_request_reply() {
    for(int i : quorum) {
        request_reply[i] = NONE;
    }
}

std::string MKWCriticalSectionAlgStrategy::get_msg() {
    std::string msg = "";
    msg += filename;
    msg += ';';
    msg += std::to_string(ME);
    msg += '#';
    msg += std::to_string(our_sequence_number);
    return msg;
}

// REQUEST message format: <filename>;<ME>#<our_sequence_number>
void MKWCriticalSectionAlgStrategy::request_resource() {
    g_mutex.lock();
    our_sequence_number = hgst_sequence_number + 1;
    client->logger.write(
        "MKWCAS[%s]: I'm requesting resource, our_sequence_number = %d\n",
        filename.c_str(), our_sequence_number);
    std::string msg = get_msg();
    for(int i : quorum) {
        client->logger.write("MKWCAS[%s]: I'm sending REQUEST to Client %d\n",
            filename.c_str(), i);
        client->sendCriticalSectionMessageToClient(
            i, CLIENT_MKW_REQUEST_CLIENT, msg, our_sequence_number);
        request_reply[i] = WAITING;
    }
    g_mutex.unlock();

    while (!can_enter_critical_section()) {
        usleep(1000);
    }
    enter_critical_section();
}

// REPLY message format: <filename>;<ME>#<our_sequence_number>
void MKWCriticalSectionAlgStrategy::release_resource() {
    g_mutex.lock();
    client->logger.write(
        "MKWCAS[%s]: I'm releasing resource\n",filename.c_str());
    reset_request_reply();
    std::string msg = get_msg();
    for(int i : quorum) {
        client->logger.write("MKWCAS[%s]: I'm sending RELEASE to Client %d\n",
            filename.c_str(), i);
        client->sendCriticalSectionMessageToClient(
            i, CLIENT_MKW_RELEASE_CLIENT, msg, our_sequence_number);
    }
    g_mutex.unlock();
}

void MKWCriticalSectionAlgStrategy::treat_message(const Packet& packet) {
    MessageType message_type = packet.getMessageType();
    switch (message_type) {
        case CLIENT_MKW_REQUEST_CLIENT:
            treat_request_message(packet);
            break;
        case CLIENT_MKW_INQUIRE_CLIENT:
            treat_inquire_message(packet);
            break;
        case CLIENT_MKW_RELINQUISH_CLIENT:
            treat_relinquish_message(packet);
            break;
        case CLIENT_MKW_FAILED_CLIENT:
            treat_failed_message(packet);
            break;
        case CLIENT_MKW_LOCKED_CLIENT:
            treat_locked_message(packet);
            break;
        case CLIENT_MKW_RELEASE_CLIENT:
            treat_release_message(packet);
            break;
        default:
            printf("Wrong MKW message type\n");
            client->logger.write("Wrong MKW message type\n");
    }
}

int MKWCriticalSectionAlgStrategy::getSequenceNumberFromMessage(
        const Packet& packet) {
    std::vector<std::string> tokens = splitString(packet.getMsg(), '#');
    int sequence_number = atoi(tokens[1].c_str());
    return sequence_number;
}

void MKWCriticalSectionAlgStrategy::treat_request_message(const Packet& request_message) {
    g_mutex.lock();
    int j = request_message.getSenderId();
    client->logger.write(
        "MKWCAS[%s]: I'm treating REQUEST message from Client %d\n",
        filename.c_str(), j);
    client->logger.writePacket(request_message);
    int their_sequence_number = getSequenceNumberFromMessage(request_message);
    hgst_sequence_number = std::max(
        hgst_sequence_number, their_sequence_number);
    std::string msg = get_msg();
    if (!is_locked) {
        is_locked = true;
        locking_request = request_message;
        client->logger.write("MKWCAS[%s]: I'm sending LOCKED to Client %d\n",
            filename.c_str(), j);
        client->sendCriticalSectionMessageToClient(
            j, CLIENT_MKW_LOCKED_CLIENT, msg, our_sequence_number);
    } else {
        waiting_queue.push(request_message);
        if (packet_comparator(request_message, locking_request)
                || (!waiting_queue.empty() && packet_comparator(
                        request_message, waiting_queue.top()))) {
            client->logger.write("MKWCAS[%s]: I'm sending FAILED to Client %d\n",
                filename.c_str(), j);
            client->sendCriticalSectionMessageToClient(
                j, CLIENT_MKW_FAILED_CLIENT, msg, our_sequence_number);
        } else {
            int locking_node_id = locking_request.getSenderId();
            if (is_inquire_replied[locking_node_id]) {
                client->logger.write("MKWCAS[%s]: I'm sending INQUIRE to Client %d\n",
                    filename.c_str(), locking_node_id);
                client->sendCriticalSectionMessageToClient(
                    locking_node_id, CLIENT_MKW_INQUIRE_CLIENT, 
                    msg, our_sequence_number);
                is_inquire_replied[locking_node_id] = false;
            } else {
                client->logger.write("MKWCAS[%s]: INQUIRE message was sent to Client %d and I'm waiting for the response\n",
                    filename.c_str(), locking_node_id);
            }
        }
    }
    g_mutex.unlock();
}

void MKWCriticalSectionAlgStrategy::treat_inquire_message(const Packet& inquire_message) {
    int j = inquire_message.getSenderId();
    client->logger.write(
        "MKWCAS[%s]: I'm treating INQUIRE message from Client %d\n",
        filename.c_str(), j);
    client->logger.writePacket(inquire_message);

    while (!can_determine_inquire_result()) {
        client->logger.write(
            "MKWCAS[%s]: INQUIRE message: can't determine what to reply because: request_reply = [", filename.c_str());
        for(int i : quorum) {
            client->logger.write("%d ", request_reply[i]);
        }
        client->logger.write("]\n");
        usleep(1000);
    }

    if (received_failed()) {
        g_mutex.lock();
        std::string msg = get_msg();
        client->logger.write("MKWCAS[%s]: I'm sending RELINQUISH to Client %d\n",
            filename.c_str(), j);
        client->sendCriticalSectionMessageToClient(
            j, CLIENT_MKW_RELINQUISH_CLIENT, msg, our_sequence_number);
        request_reply[j] = FAILED;
        g_mutex.unlock();
    } else if (received_all_locked()) {
        client->logger.write("MKWCAS[%s]:  I'm successful in lock all of my quorum. I'll send RELEASE after\n",
            filename.c_str(), j);
    }
}

void MKWCriticalSectionAlgStrategy::treat_relinquish_message(const Packet& relinquish_message) {
    g_mutex.lock();
    int j = relinquish_message.getSenderId();
    client->logger.write(
        "MKWCAS[%s]: I'm treating RELINQUISH message from Client %d\n",
        filename.c_str(), j);
    client->logger.writePacket(relinquish_message);
    Packet most_preceding_request = waiting_queue.top();
    waiting_queue.pop();
    waiting_queue.push(locking_request);
    locking_request = most_preceding_request;
    int locking_node_id = locking_request.getSenderId();
    is_inquire_replied[j] = true;
    std::string msg = get_msg();
    client->logger.write("MKWCAS[%s]: I'm sending LOCKED to Client %d\n",
        filename.c_str(), locking_node_id);
    client->sendCriticalSectionMessageToClient(
        locking_node_id, CLIENT_MKW_LOCKED_CLIENT, msg, our_sequence_number);
    g_mutex.unlock();
}

void MKWCriticalSectionAlgStrategy::treat_failed_message(const Packet& failed_message) {
    g_mutex.lock();
    int j = failed_message.getSenderId();
    client->logger.write(
        "MKWCAS[%s]: I'm treating FAILED message from Client %d\n",
        filename.c_str(), j);
    client->logger.writePacket(failed_message);
    request_reply[j] = FAILED;
    g_mutex.unlock();
}

void MKWCriticalSectionAlgStrategy::treat_locked_message(const Packet& locked_message) {
    g_mutex.lock();
    int j = locked_message.getSenderId();
    client->logger.write(
        "MKWCAS[%s]: I'm treating LOCKED message from Client %d\n",
        filename.c_str(), j);
    client->logger.writePacket(locked_message);
    request_reply[j] = LOCKED;
    g_mutex.unlock();
}

void MKWCriticalSectionAlgStrategy::treat_release_message(const Packet& release_message) {
    g_mutex.lock();
    int j = release_message.getSenderId();
    client->logger.write(
        "MKWCAS[%s]: I'm treating RELEASE message from Client %d\n",
        filename.c_str(), j);
    client->logger.writePacket(release_message);
    is_inquire_replied[j] = true;
    if (!waiting_queue.empty()) {
        locking_request = waiting_queue.top();
        waiting_queue.pop();
        int locking_node_id = locking_request.getSenderId();
        std::string msg = get_msg();
        client->logger.write(
            "MKWCAS[%s]: I'm sending LOCKED message from Client %d\n",
            filename.c_str(), locking_node_id);
        client->sendCriticalSectionMessageToClient(
            locking_node_id, CLIENT_MKW_LOCKED_CLIENT, msg, our_sequence_number);
    } else {
        is_locked = false;
        locking_request.setSenderId(-1);
    }
    g_mutex.unlock();
}

bool MKWCriticalSectionAlgStrategy::can_determine_inquire_result() {
    return received_failed() || received_all_locked();
    return true;
}

bool MKWCriticalSectionAlgStrategy::received_failed() {
    g_mutex.lock();
    bool res = false;
    for(int i : quorum) {
        if (request_reply[i] == FAILED) {
            res = true;
            break;
        }
    }
    g_mutex.unlock();
    return res;
}

bool MKWCriticalSectionAlgStrategy::received_all_locked() {
    g_mutex.lock();
    bool res = true;
    for(int i : quorum) {
        if (request_reply[i] != LOCKED) {
            res = false;
            break;
        }
    }
    g_mutex.unlock();
    return res;
}

bool MKWCriticalSectionAlgStrategy::can_enter_critical_section() {
    g_mutex.lock();
    bool all_request_reply_are_locked = true;
    std::vector<int> waiting_for;
    for(int i : quorum) {
        if (request_reply[i] != LOCKED) {
            waiting_for.push_back(i);
            all_request_reply_are_locked &= false;
        }
    }
    client->logger.write(
       "MKWCAS[%s]: Can I enter critical section? Answer is %s. I'm waiting for: [",
       filename.c_str(), (all_request_reply_are_locked ? "YES" : "NO"));
    for(int x : waiting_for) client->logger.write("%d ", x);
    client->logger.write("]. Locking request node = %d\n", locking_request.getSenderId());
    client->logger.writePacket(locking_request);
    g_mutex.unlock();
    return all_request_reply_are_locked;
}

void MKWCriticalSectionAlgStrategy::enter_critical_section() {
    client->logger.write("MKWCAS[%s]: I'm entering critical section\n", filename.c_str());
    client->enterCriticalSection();
    release_resource();
}
