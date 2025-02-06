#include "game_protoc.h"

void Greeting::serialize(char *buffer) const
{
    constexpr size_t msg_size = sizeof(msg);
    constexpr size_t uuid_size = sizeof(uuid);

    // Копирование msg в буфер
    std::memcpy(buffer, msg, msg_size);

    // Копирование uuid в буфер после msg
    std::memcpy(buffer + msg_size, uuid, uuid_size);
}

std::shared_ptr<Greeting> Greeting::deserialize(const char *data)
{
    std::shared_ptr<Greeting> header = std::make_shared<Greeting>(); // Используем std::make_shared для правильного выделения памяти
    std::memcpy(header.get(), data, sizeof(Greeting)); // Копируем данные в объект
    return header;
}

std::shared_ptr<MessageHeader> MessageHeader::deserialize(const char *data)
{
    std::shared_ptr<MessageHeader> header = std::make_shared<MessageHeader>(); // Используем std::make_shared для правильного выделения памяти
    std::memcpy(header.get(), data, sizeof(MessageHeader)); // Копируем данные в объект
    return header;
}

void ClientsList::serialize(char *buffer) const
{
}

std::shared_ptr<ClientsList> ClientsList::deserialize(const char *data)
{
    return std::shared_ptr<ClientsList>();
}

void ClientsList::populate_with_clients(const std::map<std::string, std::shared_ptr<boost::asio::ip::tcp::socket>> &clients_map)
{
}

void SRoom::serialize(char *buffer) const
{
    std::memcpy(buffer, room_name, sizeof(room_name));  // Копируем имя комнаты
    std::memcpy(buffer + sizeof(room_name), joined_uuid, sizeof(joined_uuid));  // Копируем UUID вошедшего клиента

    // Правильное смещение: room_name (128) + joined_uuid (48) = 176
    for (int i = 0; i < 5; ++i) {
        std::memcpy(buffer + sizeof(room_name) + sizeof(joined_uuid) + i * sizeof(uuids[0]), 
                    uuids[i], sizeof(uuids[i]));
    }
}

std::shared_ptr<SRoom> SRoom::deserialize(const char *data)
{
    auto room = std::make_shared<SRoom>();  // Создаём shared_ptr, чтобы память не освобождалась

    std::memcpy(room->room_name, data, sizeof(room->room_name));
    std::memcpy(room->joined_uuid, data + sizeof(room->room_name), sizeof(room->joined_uuid));

    for (int i = 0; i < 5; ++i) {
        std::memcpy(room->uuids[i], data + sizeof(room->room_name) + sizeof(room->joined_uuid) + i * sizeof(room->uuids[0]), sizeof(room->uuids[0]));
    }

    return room;
}
