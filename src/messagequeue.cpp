#include "distributed-me/messagequeue.h"

MessageQueue::MessageQueue(int max_size) {
    this->max_size = max_size;
}

int MessageQueue::getMaxSize() const {
    return max_size;
}

bool MessageQueue::push(const Packet& packet) {
    g_mutex.lock();
    if (pq.size() < max_size) {
        pq.push(packet);
        g_mutex.unlock();
        return true;
    } else{
        g_mutex.unlock();
        return false;
    }
}

void MessageQueue::pop() {
    g_mutex.lock();
    pq.pop();
    g_mutex.unlock();
}

Packet MessageQueue::top() {
    Packet front_;
    g_mutex.lock();
    front_ = pq.top();
    g_mutex.unlock();
    return front_;
}

int MessageQueue::size() {
    int sz;
    g_mutex.lock();
    sz = pq.size();
    g_mutex.unlock();
    return sz;
}

bool MessageQueue::empty() {
    return this->size() == 0;
}

void MessageQueue::print() {
    g_mutex.lock();
    auto pq_copy = pq;
    printf("\nMessageQueue = {\n");
    while (!pq_copy.empty()) {
        Packet packet = pq_copy.top();
        pq_copy.pop();
        packet.printPacket(stdout);
    }
    printf("}\n\n");
    g_mutex.unlock();
}

void MessageQueue::writeLog(Logger& logger, const char* prefix) {
    g_mutex.lock();
    auto pq_copy = pq;
    logger.write("\n%s = {\n", prefix);
    while (!pq_copy.empty()) {
        Packet packet = pq_copy.top();
        pq_copy.pop();
        logger.writePacket(packet);
    }
    logger.write("}\n\n");
    g_mutex.unlock();
}