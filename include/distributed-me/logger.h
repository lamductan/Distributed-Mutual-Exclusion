#ifndef DISTRIBUTED_ME_LOGGER_H_
#define DISTRIBUTED_ME_LOGGER_H_

#include <string>
#include <queue>
#include <mutex>

#include "distributed-me/config.h"
#include "distributed-me/packet.h"

class Logger {
private:
    char filename[MAX_FILENAME_LENGTH];
    std::mutex g_mutex;

public:
    Logger();
    Logger(const char* filename);
    std::string getFilename();
    void set(const char* filename, const char* mode="w");
    void write(const char* format, ...);
    void writePacket(const Packet& packet);
    void writeRequestQueue(std::queue<Packet>& q);
    void close();
    ~Logger();
};

#endif // DISTRIBUTED_ME_LOGGER_H_