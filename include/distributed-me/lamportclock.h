#ifndef DISTRIBUTED_ME_LAMPORT_CLOCK_H_
#define DISTRIBUTED_ME_LAMPORT_CLOCK_H_

#include <mutex>

class LamportClock {
private:
    int timestamp;
    int d;
    std::mutex g_mutex;
public:
    LamportClock(int timestamp = 0, int d = 1);
    int increaseTimestamp();
    int getD() const;
    int getTimestamp();
    int updateTimestamp(int new_timestamp);
};

#endif // DISTRIBUTED_ME_LAMPORT_CLOCK_H_