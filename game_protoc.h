#pragma once

#include <cstdint>
#include <cstring>
#include <boost/chrono.hpp>
#include "iostream"
#include <vector>
#include <mutex>
#include "iostream"
#include <memory>
#include <map>
#include <boost/asio.hpp>
#include <thread>


enum class MessageType 
{
    GREETING,
    USERSLIST,
    ADD_TO_GROUP,
    FIND_GAME
};

struct MessageHeader 
{
    MessageType type;
    uint32_t size;
    static std::shared_ptr<MessageHeader> deserialize(const char* data);
};

struct Greeting
{
    char msg[128] = {};
    char uuid[128] = {};
    void serialize(char* buffer) const;
    static std::shared_ptr<Greeting> deserialize(const char* data);
};

struct ClientsList 
{
    char cmd[128];
    char uuids[5][48];
    void serialize(char* buffer) const;
    static std::shared_ptr<ClientsList> deserialize(const char* data);
    void populate_with_clients(const std::map<std::string, std::shared_ptr<boost::asio::ip::tcp::socket>>& clients_map);
};

struct SRoom 
{
    char room_name[128] = {};
    char uuids[5][48] = {};
    char joined_uuid[48] = {};
    void serialize(char* buffer) const;
    static std::shared_ptr<SRoom> deserialize(const char* data);
};