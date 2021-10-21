#ifndef DISTRIBUTED_ME_MKW_CRITICAL_SECTION_ALG_STRATEGY_H_
#define DISTRIBUTED_ME_MKW_CRITICAL_SECTION_ALG_STRATEGY_H_

#include <map>
#include <vector>
#include <string>

#include "distributed-me/packet.h"
#include "distributed-me/messagequeue.h"
#include "distributed-me/CriticalSectionAlg/CriticalSectionAlgStrategy.h"

class Client;

class MKWCriticalSectionAlgStrategy : public CriticalSectionAlgStrategy {
private:
    enum RequestReplyType {
        NONE,
        WAITING,
        FAILED,
        LOCKED
    };

    std::string filename;
    std::vector<int> quorum;
    int our_sequence_number;
    int hgst_sequence_number;
    MessageQueue waiting_queue;
    std::map<int, RequestReplyType> request_reply;
    Packet locking_request;
    bool is_locked;
    std::vector<bool> is_inquire_replied;
    PacketComparator packet_comparator; //return true if lhs > rhs

    //<filename>;<ME>#<our_sequence_number>
    std::string get_msg();

    void treat_request_message(const Packet& request_message);
    void treat_inquire_message(const Packet& inquire_message);
    void treat_relinquish_message(const Packet& relinquish_message);
    void treat_failed_message(const Packet& failed_message);
    void treat_locked_message(const Packet& locked_message);
    void treat_release_message(const Packet& release_message);

    bool can_determine_inquire_result();
    bool received_failed();
    bool received_all_locked();
    void reset_request_reply();

    static int getSequenceNumberFromMessage(const Packet& packet);

public:
    MKWCriticalSectionAlgStrategy(
        Client* client, const std::string& filename,
        const std::vector<int>& quorum); 
    void request_resource();
    void release_resource();
    void treat_message(const Packet& message);
    bool can_enter_critical_section();
    void enter_critical_section();
    void write_log(const char* format, ...);
};

#endif // DISTRIBUTED_ME_MKW_CRITICAL_SECTION_ALG_STRATEGY_H_