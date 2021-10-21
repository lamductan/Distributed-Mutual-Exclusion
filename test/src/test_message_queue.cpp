#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <thread>
#include <set>
#include <vector>
#include <mutex>
#include <gtest/gtest.h>

#include "distributed-me/messagequeue.h"
#include "distributed-me/lamportclock.h"

struct TestMessageQueue
    : public ::testing::Test
{
    static MessageQueue* messageQueue;

    static void SetUpTestSuite() {
        messageQueue = new MessageQueue();
    }

    static void TearDownTestSuite() {
        delete messageQueue;
    }
};

MessageQueue* TestMessageQueue::messageQueue = NULL;

TEST_F(TestMessageQueue, TestSimpleInitialization) {
    EXPECT_EQ(messageQueue->getMaxSize(), MESSAGE_QUEUE_MAX_SIZE);
}

MessageQueue message_queue;
void addMessage(int t, int thread_id) {
    Packet packet(CLIENT_ENQUIRY, thread_id, 0, 0);
    LamportClock lamportClock;
    for(int i = 0; i < t; ++i) {
        packet.setTimestamp(lamportClock.getTimestamp());
        message_queue.push(packet);
        lamportClock.increaseTimestamp();
    }
}

bool isIncreasing(const std::vector<Packet>& v) {
    PacketComparator packet_comparator;
    for(int i = 1; i < v.size(); ++i) {
        if (!packet_comparator(v[i], v[i - 1]))
            return false;
    }
    return true;
}

TEST_F(TestMessageQueue, TestMessageQueue) {
    int t = 10;
    std::thread t0(addMessage, t, 0);
    std::thread t1(addMessage, t, 1);
    t0.join();
    t1.join();

    std::vector<Packet> v;
    while (!message_queue.empty()) {
        Packet top = message_queue.top();
        message_queue.pop();
        v.push_back(top);
    }
    EXPECT_TRUE(isIncreasing(v));
}
