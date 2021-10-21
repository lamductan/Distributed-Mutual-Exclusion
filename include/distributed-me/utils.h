#ifndef DISTRIBUTED_ME_UTILS_H_
#define DISTRIBUTED_ME_UTILS_H_

#include <stdio.h>
#include <string>
#include <vector>

void intToChar(int x, char*);
int charToInt(char* s);
void printMessage(FILE* out, char* message, int len);
void check(bool t, const char* err_msg);
void setTimeoutSockfd(int sockfd, int timeout_in_sec);
std::vector<std::string> getListFiles(const char* directory);
std::string _readLastLineOfFile(const std::string& filepath);
int countLine(const std::string& filePath);
void _writeToFile(const std::string& filepath, const std::string& content_to_write);
void getIpAddrsAndPortsFromConfig(
    const char* config_file, 
    std::vector<std::string>& ip_addr_s, 
    std::vector<int>& port_s);
std::vector<std::vector<int>> getQuorumsFromConfig(const char* config_file);
void _generateRandomInputFile(
    const std::vector<std::string>& list_files,
    const char* input_filename, int n_requests, int n_servers, int client_id);

std::vector<std::string> splitString(const std::string& s, char delimiter);

#endif // DISTRIBUTED_ME_UTILS_H_