#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <gtest/gtest.h>

#include "distributed-me/utils.h"
#include "distributed-me/packet.h"

struct TestPacket
    : public ::testing::Test
{
    static Packet packet;
    static Packet packet1;

    static void SetUpTestSuite() {
        Packet packet_1(CLIENT_ENQUIRY, 0, 1, 2);
        packet = packet_1;
        Packet packet_2(CLIENT_ENQUIRY, 0, 1, 2, "lamductan");
        packet1 = packet_2;
    }

    static void TearDownTestSuite() {
    }
};

Packet TestPacket::packet;
Packet TestPacket::packet1;


TEST_F(TestPacket, PacketToMessage) {
    char message[1024];
    packet.toMessage(message, 1024);
    char correct_message[20] = {
        20, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        1, 0, 0, 0,
        2, 0, 0, 0
    };
    EXPECT_TRUE(memcmp(
        correct_message, message, 20) == 0);
}

TEST_F(TestPacket, PacketToMessage1) {
    char message[1024];
    packet1.toMessage(message, 1024);
    char correct_message[29] = {
        29, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        1, 0, 0, 0,
        2, 0, 0, 0,
        'l', 'a', 'm', 'd', 'u', 'c', 't', 'a', 'n'
    };
    EXPECT_TRUE(memcmp(
        correct_message, message, sizeof(correct_message)) == 0);
}

TEST_F(TestPacket, MessageToPacket) {
    char m[20] = {
        20, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        1, 0, 0, 0,
        2, 0, 0, 0
    };
    Packet created_packet = MessageToPacket(m);
    EXPECT_EQ(
        packet.getMessageType(),
        created_packet.getMessageType()
    );
    EXPECT_EQ(
        packet.getSenderId(),
        created_packet.getSenderId()
    );
    EXPECT_EQ(
        packet.getReceiverId(),
        created_packet.getReceiverId()
    );
    EXPECT_EQ(
        packet.getTimestamp(),
        created_packet.getTimestamp()
    );
    EXPECT_EQ(
        packet.getMsg(),
        created_packet.getMsg()
    );
}

TEST_F(TestPacket, MessageToPacket1) {
    char m[29] = {
        29, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        1, 0, 0, 0,
        2, 0, 0, 0,
        'l', 'a', 'm', 'd', 'u', 'c', 't', 'a', 'n'
    };
    Packet created_packet = MessageToPacket(m);
    EXPECT_EQ(
        packet1.getMessageType(),
        created_packet.getMessageType()
    );
    EXPECT_EQ(
        packet1.getSenderId(),
        created_packet.getSenderId()
    );
    EXPECT_EQ(
        packet1.getReceiverId(),
        created_packet.getReceiverId()
    );
    EXPECT_EQ(
        packet1.getTimestamp(),
        created_packet.getTimestamp()
    );
    EXPECT_EQ(
        packet1.getMsg(),
        created_packet.getMsg()
    );
}

TEST_F(TestPacket, splitReadDataFromSocket) {
    Packet packet_a(CLIENT_ENQUIRY, 0, 1, 2, "lamductan");
    Packet packet_b(CLIENT_ENQUIRY, 5, 6, 7, "utdallas 123");
    char buffer_a[1000];
    char buffer_b[1000];
    packet_a.toMessage(buffer_a, 1000);
    packet_b.toMessage(buffer_b, 1000);

    memcpy(buffer_a + packet_a.getMessageLength(), buffer_b, packet_b.getMessageLength());
    int valread = packet_a.getMessageLength() + packet_b.getMessageLength();

    std::vector<Packet> vPackets = splitReadDataFromSocket(buffer_a, valread);

    ASSERT_EQ(int(vPackets.size()), 2);
    ASSERT_TRUE(packet_a.equal(vPackets[0]));
    ASSERT_TRUE(packet_b.equal(vPackets[1]));
}