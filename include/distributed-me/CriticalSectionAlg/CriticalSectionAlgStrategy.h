#ifndef DISTRIBUTED_ME_CRITICAL_SECTION_ALG_STRATEGY_H_
#define DISTRIBUTED_ME_CRITICAL_SECTION_ALG_STRATEGY_H_

#include <mutex>

#include "distributed-me/packet.h"

class Client;

enum CriticalSectionAlgType {
    LAMPORT_CRITICAL_SECTION_ALG,
    RA_CRITICAL_SECTION_ALG,
    MKW_CRITICAL_SECTION_ALG
};

class CriticalSectionAlgStrategy {
protected:
    int ME;
    int n;
    Client* client;
    std::mutex g_mutex;

public:
    virtual void set_n_clients(int n_clients);
    virtual void request_resource() = 0;
    virtual void release_resource() = 0;
    virtual void treat_message(const Packet& packet) = 0;
    virtual bool can_enter_critical_section() = 0;
    virtual void enter_critical_section() = 0;
};

#endif // DISTRIBUTED_ME_CRITICAL_SECTION_ALG_STRATEGY_H_