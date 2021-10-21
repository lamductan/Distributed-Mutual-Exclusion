#include <stdlib.h>
#include <string>

#include "distributed-me/utils.h"
#include "distributed-me/packet.h"

Packet::Packet() {
    message_type = CLIENT_ENQUIRY;
    sender_id = 0;
    receiver_id = 0;
    timestamp = 0;
    msg = "";
}

Packet::Packet(
        MessageType message_type,
        int sender_id,
        int receiver_id,
        int timestamp,
        std::string msg) {
    this->message_type = message_type;
    this->sender_id = sender_id;
    this->receiver_id = receiver_id;
    this->timestamp = timestamp;
    this->msg = msg;
}

Packet::Packet(const Packet& packet) {
    this->message_type = packet.message_type;
    this->sender_id = packet.sender_id;
    this->receiver_id = packet.receiver_id;
    this->timestamp = packet.timestamp;
    this->msg = packet.msg;
}

MessageType Packet::getMessageType() const {
    return message_type;
}

int Packet::getSenderId() const { return sender_id; }

int Packet::getReceiverId() const { return receiver_id; }

int Packet::getTimestamp() const { return timestamp; }

std::string Packet::getMsg() const { return msg; }

int Packet::getPacketLength() const {
    return sizeof(message_type) + sizeof(sender_id) 
            + sizeof(receiver_id) + sizeof(timestamp)
            + msg.length(); 
}

int Packet::getMessageLength() const {
    return sizeof(int) + getPacketLength();
}

void Packet::setMessageType(MessageType message_type) {
    this->message_type = message_type;
}

void Packet::setSenderId(int sender_id) {
    this->sender_id = sender_id;
}

void Packet::setReceiverId(int receiver_id) {
    this->receiver_id = receiver_id;
}

void Packet::setTimestamp(int timestamp) {
    this->timestamp = timestamp;
}

void Packet::setMsg(const std::string& msg) {
    this->msg = msg;
}

int Packet::toMessage(char* buffer, int buffer_size) const {
    int message_len = getMessageLength();
    intToChar(message_len, buffer + MESSAGE_LEN_OFFSET);
    intToChar(message_type, buffer + MESSAGE_TYPE_OFFSET);
    intToChar(sender_id, buffer + MESSAGE_SENDER_ID_OFFSET);
    intToChar(receiver_id, buffer + MESSAGE_RECEIVER_ID_OFFSET);
    intToChar(timestamp, buffer + MESSAGE_TIMESTAMP_OFFSET);

    int n_available_bytes_in_buffer = buffer_size - MESSAGE_MSG_OFFSET;
    if (n_available_bytes_in_buffer >= msg.length()) {
        msg.copy(buffer + MESSAGE_MSG_OFFSET, msg.length());
        return 0;
    } else {
        msg.copy(buffer + MESSAGE_MSG_OFFSET, n_available_bytes_in_buffer);
        return msg.length() - n_available_bytes_in_buffer;
    }
}

void Packet::printPacket(FILE* out) const {
    fprintf(out, "{\n");
    fprintf(out, "\tpacket.message_type = %d\n", message_type);
    fprintf(out, "\tpacket.sender_id = %d\n", sender_id);
    fprintf(out, "\tpacket.receiver_id = %d\n", receiver_id);
    fprintf(out, "\tpacket.timestamp = %d\n", timestamp);
    fprintf(out, "\tpacket.msg = %s\n", msg.c_str());
    fprintf(out, "}\n");
}

bool Packet::equal(const Packet& packet) {
    return (
        this->message_type == packet.message_type &&
        this->sender_id == packet.sender_id &&
        this->receiver_id == packet.receiver_id &&
        this->timestamp == packet.timestamp &&
        this->msg == packet.msg);
}

Packet MessageToPacket(char* message) {
    int message_len = charToInt(message + MESSAGE_LEN_OFFSET);
    MessageType message_type = static_cast<MessageType>(
        charToInt(message + MESSAGE_TYPE_OFFSET));
    int sender_id   = charToInt(message + MESSAGE_SENDER_ID_OFFSET);
    int receiver_id = charToInt(message + MESSAGE_RECEIVER_ID_OFFSET);
    int timestamp   = charToInt(message + MESSAGE_TIMESTAMP_OFFSET);
    int msg_len = message_len - MESSAGE_MSG_OFFSET;
    //printf("msg_len = %d\n", msg_len);
    std::string msg(
        message + MESSAGE_MSG_OFFSET,
        message_len - MESSAGE_MSG_OFFSET);
    return Packet(
        message_type, sender_id, 
        receiver_id, timestamp, msg);
}

std::vector<Packet> splitReadDataFromSocket(char* data, int valread) {
    int num_read = 0;
    std::vector<Packet> res;
    while (num_read < valread) {
        Packet packet = MessageToPacket(data + num_read);
        res.push_back(packet);
        num_read += packet.getMessageLength();
    }

    return res;
}