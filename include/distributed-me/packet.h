#ifndef DISTRIBUTED_ME_PACKET_H_
#define DISTRIBUTED_ME_PACKET_H_

#include <stdio.h>
#include <string>
#include <vector>

enum MessageType {
    CLIENT_ENQUIRY,
    CLIENT_REQUEST_READ,
    CLIENT_REQUEST_WRITE,
    CLIENT_ECHO,

    SERVER_REQUEST_SERVER,
    SERVER_REPLY_SERVER,
    SERVER_REPLY_ENQUIRY_CLIENT,
    SERVER_REPLY_READ_CLIENT,
    SERVER_REPLY_WRITE_CLIENT,
    SERVER_ECHO,
    
    CLIENT_DONE_ALL_REQUEST,
    SERVER_RECEIVED_ALL_DONE_REQUESTS,
    
    CLIENT_REQUEST_CS_CLIENT,
    CLIENT_REPLY_CS_CLIENT,
    
    CLIENT_MKW_REQUEST_CLIENT,
    CLIENT_MKW_LOCKED_CLIENT,
    CLIENT_MKW_INQUIRE_CLIENT,
    CLIENT_MKW_RELINQUISH_CLIENT,
    CLIENT_MKW_FAILED_CLIENT,
    CLIENT_MKW_RELEASE_CLIENT
};

const int MESSAGE_LEN_OFFSET         = 0;
const int MESSAGE_TYPE_OFFSET        = MESSAGE_LEN_OFFSET + sizeof(int);
const int MESSAGE_SENDER_ID_OFFSET   = MESSAGE_TYPE_OFFSET + sizeof(MessageType);
const int MESSAGE_RECEIVER_ID_OFFSET = MESSAGE_SENDER_ID_OFFSET + sizeof(int);
const int MESSAGE_TIMESTAMP_OFFSET   = MESSAGE_RECEIVER_ID_OFFSET + sizeof(int);
const int MESSAGE_MSG_OFFSET         = MESSAGE_TIMESTAMP_OFFSET + sizeof(int);

class Packet {
private:
    MessageType message_type;
    int sender_id;
    int receiver_id;
    int timestamp;
    std::string msg;

public:
    Packet();
    Packet(
        MessageType message_type,
        int sender_id,
        int receiver_id,
        int timestamp,
        std::string msg="");
    Packet(const Packet& packet);
    MessageType getMessageType() const;
    int getSenderId() const;
    int getReceiverId() const;
    int getTimestamp() const;
    std::string getMsg() const;
    int getPacketLength() const;
    int getMessageLength() const;

    void setMessageType(MessageType message_type);
    void setSenderId(int sender_id);
    void setReceiverId(int receiver_id);
    void setTimestamp(int timestamp);
    void setMsg(const std::string& msg);

    // return the number of byte remained in the 
    // packet that need to continue to read.
    // Now can only get message < 1024 bytes
    int toMessage(char* buffer, int buffer_size) const;

    /**
     * Function to deserialize an array of char into packet
     * param[in]: message, an array of char
     * 
     * Return pointer of the newly created packet corresponding to 
     * the message.
     */
    void printPacket(FILE* out) const;

    bool equal(const Packet& packet);
};

/**
 * Function to serialize a packet into an array of char
 * param[in]: packet, the input packet
 * 
 * Return the string corresponding to the packet.
 * Structure of message:
 *    First 4 bytes: len of array (to deal with character \0 
 *                   of char array)
 *    Next 4 bytes : message_type of packet
 *    Next 4 bytes : sender_id
 *    Next 4 bytes : receiver_id
 *    Next 4 bytes : timestamp
 *    Remaining bytes: msg of the packet
 */
Packet MessageToPacket(char* message);

std::vector<Packet> splitReadDataFromSocket(char* data, int valread);

#endif // DISTRIBUTED_ME_PACKET_H_