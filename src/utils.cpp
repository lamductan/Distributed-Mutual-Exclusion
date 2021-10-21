#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h> 
#include <sys/time.h>
#include <string>
#include <sstream>

#include "distributed-me/utils.h"

typedef union {
    char c[4];
    int i;
} union_int_char_array;

void intToChar(int x, char* res) {
    union_int_char_array u;
    u.i = x;
    memcpy(res, u.c, sizeof(int));
}

int charToInt(char* s) {
    union_int_char_array u;
    memcpy((&u)->c, s, 4);
    return u.i;
}

void printMessage(FILE* out, char* message, int len) {
    for(int i = 0; i < len; ++i) {
        fprintf(out, "%d ", message[i]);
    }
}

void check(bool t, const char* err_msg) {
    if (!t) {
		perror(err_msg); 
		exit(EXIT_FAILURE); 
    }
}

void setTimeoutSockfd(int sockfd, int timeout_in_sec) {
    if (timeout_in_sec <= 0) 
        return;
    struct timeval tv;
    tv.tv_sec = timeout_in_sec;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
}

std::vector<std::string> getListFiles(const char* directory) {
    std::vector<std::string> list_files;
    DIR* dr = opendir(directory);
    struct dirent *de;
    while ((de = readdir(dr)) != NULL) {
        std::string filename = de->d_name;
        if (filename == "." || filename == "..") {
            continue;
        }
        list_files.push_back(filename);
    }
    return list_files;
}

std::string _readLastLineOfFile(const std::string& filePath) {
    FILE* f = fopen(filePath.c_str(), "r");
    if (f != NULL) {
        char buffer[1024], prev_buffer[1024];
        while(fgets(buffer, 1024, f) != NULL) {
            size_t len = strlen(buffer);
            if (len == 0) break;
            if (len > 0 && buffer[0] == '\n') continue;
            strcpy(prev_buffer, buffer);
        }
        fclose(f);
        size_t len = strlen(prev_buffer);
        if (prev_buffer[len - 1] == '\n')
            prev_buffer[len - 1] = '\0';
        return std::string(prev_buffer);
    }
    return "";
}

int countLine(const std::string& filePath) {
    FILE* f = fopen(filePath.c_str(), "r");
    int n_lines = 0;
    if (f != NULL) {
        char buffer[1024];
        while(fgets(buffer, 1024, f) != NULL) {
            size_t len = strlen(buffer);
            if (len == 0) break;
            if (len > 0 && buffer[0] == '\n') continue;
            ++n_lines;
        }
        fclose(f);
        return n_lines;
    }
    return -1;
}

void _writeToFile(
        const std::string& filepath, const std::string& content_to_write) {
    FILE* f = fopen(filepath.c_str(), "a");
    if (f != NULL) { 
        fwrite(
            content_to_write.c_str(), 1,
            content_to_write.length(), f);
        fwrite("\n", 1, 1, f);
        fclose(f);
    }
}

void getIpAddrsAndPortsFromConfig(
        const char* config_file, 
        std::vector<std::string>& ip_addr_s, 
        std::vector<int>& port_s) {
    FILE* f = fopen(config_file, "r");
    printf("config file = %s\n", config_file);
    if (f != NULL) {
        char ip_addr[16];
        int port;
        int bufferLength = 255;
        char buffer[bufferLength];
        while(fgets(buffer, bufferLength, f) != NULL) {
            if (strlen(buffer) <= 1) break;
            sscanf(buffer, "%s %d", ip_addr, &port);
            ip_addr_s.push_back(std::string(ip_addr));
            port_s.push_back(port);
        }
    }
    fclose(f);

    // for(int i = 0; i < ip_addr_s.size(); ++i) {
    //     printf("%s:%d\n", ip_addr_s[i].c_str(), port_s[i]);
    // }
}

std::vector<std::vector<int>> getQuorumsFromConfig(const char* config_file) {
    FILE* f = fopen(config_file, "r");
    std::vector<std::vector<int>> quorums;
    if (f != NULL) {
        int bufferLength = 255;
        char buffer[bufferLength];
        while(fgets(buffer, bufferLength, f) != NULL) {
            if (strlen(buffer) <= 1) break;
        }
        while(fgets(buffer, bufferLength, f) != NULL) {
            if (strlen(buffer) <= 1) break;
            std::string s = std::string(buffer);
            std::vector<std::string> v = splitString(s, ' ');
            std::vector<int> quorum;
            for(int i = 0; i < v.size(); ++i) {
                int q = atoi(v[i].c_str());
                quorum.push_back(q);
            }
            quorums.push_back(quorum);
        }
    }
    fclose(f);
    return quorums;
}

void _generateRandomInputFile(
        const std::vector<std::string>& list_files,
        const char* input_filename, int n_requests, int n_servers, int client_id) {
    struct timeval time; 
    gettimeofday(&time,NULL);
    srand(((time.tv_sec + client_id) * 1000) + (time.tv_usec / 1000));
    FILE* f = fopen(input_filename, "w");
    int n_files = list_files.size();
    const int N_REQUEST_TYPE = 2;
    const char* request_types[N_REQUEST_TYPE] = {"read", "write"};
    char request_str[100];
    for(int i = 0; i < n_requests; ++i) {
        int request_type_id = rand() % N_REQUEST_TYPE;
        const char* request_type = request_types[request_type_id];
        int file_id = rand() % n_files;
        std::string request_file = list_files[file_id];
        if (strcmp(request_type, "read")) {
            int server_id = rand() % n_servers;
            snprintf(
                request_str, 100, "read %s %d\n", 
                request_file.c_str(), server_id);
            fwrite(request_str, 1, strlen(request_str), f);
        } else {
            snprintf(
                request_str, 100, "write %s\n", 
                request_file.c_str());
            fwrite(request_str, 1, strlen(request_str), f);
        }
    }
    fclose(f);
}

std::vector<std::string> splitString(const std::string& s, char delimiter) {
    std::stringstream ss(s);
    std::string token;
    std::vector<std::string> tokens;
    while (getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}
