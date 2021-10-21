#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <thread>
#include <set>
#include <vector>
#include <mutex>
#include <gtest/gtest.h>

#include "distributed-me/lamportclock.h"

struct TestLamportClock
    : public ::testing::Test
{
    static LamportClock* lamportClock;

    static void SetUpTestSuite() {
        lamportClock = new LamportClock();
    }

    static void TearDownTestSuite() {
        delete lamportClock;
    }
};

LamportClock* TestLamportClock::lamportClock = NULL;

TEST_F(TestLamportClock, TestSimpleInitialization) {
    EXPECT_EQ(lamportClock->getD(), 1);
}

TEST_F(TestLamportClock, TestSimpleInitialization1) {
    EXPECT_EQ(lamportClock->getTimestamp(), 0);
}

std::vector<int> v[2];
void increaseClock(int t, int thread_id) {
    v[thread_id].clear();
    for(int i = 0; i < t; ++i) {
        v[thread_id].push_back(
            TestLamportClock::lamportClock->increaseTimestamp());
    }
}

TEST_F(TestLamportClock, TestClock) {
    int t = 10;
    std::thread t0(increaseClock, t, 0);
    std::thread t1(increaseClock, t, 1);
    t0.join();
    t1.join();

    std::set<int> s;
    for(int i = 0; i < 2; ++i) {
        for(int x: v[i]) s.insert(x);
    }
    EXPECT_EQ(s.size(), 2*t);
}