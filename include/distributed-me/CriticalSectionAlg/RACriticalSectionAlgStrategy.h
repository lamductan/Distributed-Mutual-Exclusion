#ifndef DISTRIBUTED_ME_RA_CRITICAL_SECTION_ALG_STRATEGY_H_
#define DISTRIBUTED_ME_RA_CRITICAL_SECTION_ALG_STRATEGY_H_

#include <vector>
#include <string>

#include "distributed-me/packet.h"
#include "distributed-me/CriticalSectionAlg/CriticalSectionAlgStrategy.h"

class Client;

class RACriticalSectionAlgStrategy : public CriticalSectionAlgStrategy {
private:
    std::string filename;
    int our_sequence_number;
    int hgst_sequence_number;
    std::vector<bool> A;
    bool is_using;
    bool is_waiting;
    std::vector<bool> reply_deferred;

    //<filename>;<ME>#<our_sequence_number>
    std::string get_msg();

    void treat_reply_message(int j);
    void treat_request_message(int their_sequence_number, int j);
public:
    RACriticalSectionAlgStrategy(Client* client, const std::string& filename);
    void set_n_clients(int n_clients);
    void request_resource();
    void release_resource();
    void treat_message(const Packet& packet);
    bool can_enter_critical_section();
    void enter_critical_section();
    void write_log(const char* format, ...);
    bool is_waiting_or_using_critical_section();
};

#endif // DISTRIBUTED_ME_RA_CRITICAL_SECTION_ALG_STRATEGY_H_