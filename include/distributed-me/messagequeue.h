#ifndef DISTRIBUTED_ME_MESSAGE_QUEUE_H_
#define DISTRIBUTED_ME_MESSAGE_QUEUE_H_

#include <queue>
#include <mutex>
#include <vector>

#include "distributed-me/packet.h"
#include "distributed-me/logger.h"

struct PacketComparator {
    bool operator()(const Packet& lhs, const Packet& rhs) {
        return ((lhs.getTimestamp() > rhs.getTimestamp()) 
        || (lhs.getTimestamp() == rhs.getTimestamp() 
            && lhs.getSenderId() > rhs.getSenderId()));
    }
};

class MessageQueue {
private:
    std::priority_queue< Packet, std::vector<Packet>, PacketComparator > pq;
    int max_size;
    std::mutex g_mutex;
public:
    MessageQueue(int max_size = MESSAGE_QUEUE_MAX_SIZE);
    int getMaxSize() const;
    bool push(const Packet& packet);
    void pop();
    Packet top();
    int size();
    bool empty();
    void print();
    void writeLog(Logger& logger, const char* prefix);
};

#endif // DISTRIBUTED_ME_MESSAGE_QUEUE_H_