#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <thread>
#include <set>
#include <vector>
#include <mutex>
#include <gtest/gtest.h>

#include "distributed-me/logger.h"
#include "distributed-me/lamportclock.h"

const char* FILENAME = "../logs/test_log.txt";

struct TestLogger
    : public ::testing::Test
{
    static Logger* logger;
    static LamportClock* lamportClock;

    static void SetUpTestSuite() {
        logger = new Logger(FILENAME);
        fprintf(stderr, "open log file\n");
        lamportClock = new LamportClock();
    }

    static void TearDownTestSuite() {
        delete logger;
        delete lamportClock;
    }
};

Logger* TestLogger::logger = NULL;
LamportClock* TestLogger::lamportClock = NULL;

void writeLog(int n_runs, int thread_id) {
    for(int i = 0; i < n_runs; ++i) {
        int timestamp = TestLogger::lamportClock->increaseTimestamp();
        TestLogger::logger->write("%d %d\n", thread_id, timestamp);
    }
}

bool check_log_file(int n_threads, int n_runs) {
    FILE* f = fopen(FILENAME, "r");
    std::set<int> set_thread_id;
    std::set<int> set_timestamp;
    if (f != NULL) {
        int bufferLength = 255;
        char buffer[bufferLength];
        int thread_id, timestamp;
        while(fgets(buffer, bufferLength, f) != NULL) {
            if (strlen(buffer) <= 1) continue;
            sscanf(buffer, "%d %d", &thread_id, &timestamp);
            set_thread_id.insert(thread_id);
            set_timestamp.insert(timestamp);
        }
    }
    fclose(f);
    return (set_thread_id.size() == n_threads
            && set_timestamp.size() == n_runs*n_threads);
}

TEST_F(TestLogger, TestLogger) {
    int n_runs = 10;
    int n_threads = 2;
    for(int i = 0; i < n_threads; ++i) {
        std::thread t(writeLog, n_runs, i);
        t.join();
    }
    TestLogger::logger->close();
    EXPECT_TRUE(check_log_file(n_threads, n_runs));
}

// TEST_F(TestLogger, TestLogger) {
//     int n_runs = 10;
//     int n_threads = 2;
//     std::vector<std::thread> v_threads;
//     for(int i = 0; i < n_threads; ++i) {
//         std::thread t(writeLog, n_runs, i);
//         v_threads.push_back(std::move(t));
//     }
//     for(int i = 0; i < n_threads; ++i) {
//         v_threads[i].join();
//     }
//     TestLogger::logger->close();
//     EXPECT_TRUE(check_log_file(n_threads, n_runs));
// }