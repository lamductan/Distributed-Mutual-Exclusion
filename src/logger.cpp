#include <stdarg.h>
#include <string.h>

#include "distributed-me/logger.h"

Logger::Logger() { set("log.txt"); };

Logger::Logger(const char* filename) { set(filename); }

std::string Logger::getFilename() {
    return std::string(filename);
}

void Logger::set(const char* filename, const char* mode) {
    strcpy(this->filename, filename);
    FILE* f = fopen(filename, mode);
    fclose(f);
}

void Logger::write(const char* format, ...) {
    g_mutex.lock();
    FILE* f = fopen(filename, "a");
    va_list args;
    va_start(args, format);
    vfprintf(f, format, args);
    va_end(args);
    fclose(f);
    g_mutex.unlock();
}

void Logger::writePacket(const Packet& packet) {
    g_mutex.lock();
    FILE* f = fopen(filename, "a");
    packet.printPacket(f);
    fclose(f);
    g_mutex.unlock();
}

void Logger::writeRequestQueue(std::queue<Packet>& q) {
    std::queue<Packet> q1 = q;
    write("\nrequest_queue = {\n");
    while (!q1.empty()) {
        Packet packet = q1.front();
        q1.pop();
        writePacket(packet);
    }
    write("\n\n");
}

void Logger::close() {}

Logger::~Logger() {}