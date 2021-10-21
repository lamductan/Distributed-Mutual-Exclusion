#include "distributed-me/lamportclock.h"

LamportClock::LamportClock(int timestamp, int d) {
    this->timestamp = timestamp;
    this->d = d;
}

int LamportClock::getD() const { return d; }

int LamportClock::getTimestamp() {
    int current_timestamp = 0;
    g_mutex.lock();
    current_timestamp = timestamp;
    g_mutex.unlock();
    return current_timestamp;
}

int LamportClock::increaseTimestamp() {
    int current_timestamp;
    g_mutex.lock();
    timestamp += d;
    current_timestamp = timestamp;
    g_mutex.unlock();
    return current_timestamp;
}

int LamportClock::updateTimestamp(int new_timestamp) {
    g_mutex.lock();
    int ret;
    timestamp = std::max(timestamp + d, new_timestamp);
    ret = timestamp;
    g_mutex.unlock();
    return ret;
}