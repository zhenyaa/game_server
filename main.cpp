#include "iostream"
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <memory>
#include <cstring>
#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp> 
#include <boost/lexical_cast.hpp>
#include "user_data.h"
#include <boost/asio/buffered_read_stream.hpp>
#include "game_protoc.h"
#include <boost/uuid/uuid.hpp>       
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp> 
using namespace boost::asio;
using ip::tcp;

const char* message_type_to_string(MessageType type) {
    switch (type) {
        case MessageType::GREETING: return "GREETING";
        case MessageType::USERSLIST: return "USERLIST";
        case MessageType::ADD_TO_GROUP: return "ADD_TO_GROUP";
        case MessageType::FIND_GAME: return "FIND_GAME";
        default: return "UNKNOWN";
    }
}

class Client;

class Room
{
   private:
    std::string name;  // Use std::string for easier handling
    std::array<std::shared_ptr<Client>, 5> clients{}; // Fixed-size array of clients

public:
    Room() = default;

    explicit Room(const std::string& roomName) : name(roomName) {}

    void setName(const std::string& newName) {
        name = newName;
    }

    std::string getName() const {
        return name;
    }

    std::array<std::shared_ptr<Client>, 5>& getClients() {
        return clients;
    }

    const std::array<std::shared_ptr<Client>, 5>& getClients() const {
        return clients;
    }

    auto begin() {
        return clients.begin();
    }

    auto end() {
        return clients.end();
    }

    void addClient(std::shared_ptr<Client> client) {
        for (auto& c : clients) {
            if (!c) { // Look for an empty slot (nullptr)
                c = client;
                std::cout << "Client added to the room: " << name << std::endl;
                return;
            }
        }
        std::cout << "Room is full!" << std::endl;  // Optional: could throw an exception instead
    }

    void removeClient(std::shared_ptr<Client> client) {
        for (auto& c : clients) {
            if (c == client) {
                c.reset();  // Remove client from room
                std::cout << "Client removed from the room: " << name << std::endl;
                return;
            }
        }
        std::cout << "Client not found in the room: " << name << std::endl;
    }

    // Optional: method to display current clients in the room
    void displayClients() const {
        std::cout << "Clients in the room: " << name << std::endl;
        for (size_t i = 0; i < clients.size(); ++i) {
            if (clients[i]) {
                std::cout << "Client " << i + 1 << " is present." << std::endl;
            }
        }
    }
};


class ClientStore
{
public:
    static ClientStore& getInstance() {
        static ClientStore instance;
        return instance;
    }

    // Регистрация нового клиента
    void registerClient(const boost::uuids::uuid& uuid, std::shared_ptr<Client> client) {
        std::lock_guard<std::mutex> lock(mutex_);
        clients_[uuid] = std::move(client);
    }

    // Поиск клиента по UUID
    std::shared_ptr<Client> findClient(const boost::uuids::uuid& uuid) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = clients_.find(uuid);
        return (it != clients_.end()) ? it->second : nullptr;
    }

    std::shared_ptr<Client> findClient(const std::string& uuid_str) {
    try {
        boost::uuids::string_generator gen;
        boost::uuids::uuid uuid = gen(uuid_str);  // Конвертация строки в UUID

        std::lock_guard<std::mutex> lock(mutex_);
        auto it = clients_.find(uuid);
        return (it != clients_.end()) ? it->second : nullptr;
    } catch (const std::exception& e) {
        std::cerr << "Invalid UUID format: " << uuid_str << " (" << e.what() << ")" << std::endl;
        return nullptr;
    }
}

    // Удаление клиента
    void removeClient(const boost::uuids::uuid& uuid) {
        std::lock_guard<std::mutex> lock(mutex_);
        clients_.erase(uuid);
    }

private:
    std::map<boost::uuids::uuid, std::shared_ptr<Client>> clients_;
    std::mutex mutex_;

    ClientStore() = default;                         // Приватный конструктор
    ~ClientStore() = default;                        // Приватный деструктор
    ClientStore(const ClientStore&) = delete;        // Запрещаем копирование
    ClientStore& operator=(const ClientStore&) = delete;
};


class Client : public std::enable_shared_from_this<Client>
{
    private:
        boost::uuids::uuid uuid;
    public:
    std::shared_ptr<boost::asio::ip::tcp::socket> socket;
    std::shared_ptr<Room> room;
    char header_buffer[sizeof(MessageHeader)];

    Client(std::shared_ptr<boost::asio::ip::tcp::socket> socket) : socket(std::move(socket))
    {
        std::cout<<"init Client"<<std::endl;
        std::shared_ptr<MessageHeader> m_header = MessageHeader::deserialize(header_buffer);
        uuid = boost::uuids::random_generator()();
        std::cout<<"client: "<<boost::lexical_cast<std::string>(uuid)<<" added"<<std::endl;
      
    }

    ~Client() {
        std::cout << "Client " << boost::lexical_cast<std::string>(uuid) << " disconnected." << std::endl;
        ClientStore::getInstance().removeClient(uuid);
    }

    boost::uuids::uuid getUuid(){
        return uuid;
    }

    void authorise(){
        std::cout<<"init Client"<<std::endl;
        ClientStore::getInstance().registerClient(uuid, shared_from_this());
    }

    template <typename T>
    void send_message(MessageType type, const T& data) {
        MessageHeader header;
        header.type = type;
        header.size = sizeof(T);

        char buffer[sizeof(MessageHeader) + sizeof(T)];
        std::memcpy(buffer, &header, sizeof(MessageHeader));
        data.serialize(buffer + sizeof(MessageHeader));
        boost::asio::write(*socket, boost::asio::buffer(buffer, sizeof(MessageHeader) + sizeof(T)));
    }

    void greeting(){
        std::shared_ptr<Greeting> g = std::make_shared<Greeting>();
        snprintf(g->msg, sizeof(g->msg), "HELLO");
        std::memset(g->uuid, 0, sizeof(g->uuid));
        std::cout << "size of greeting: " << sizeof(*g) << std::endl;
        send_message(MessageType::GREETING, *g);
    }
    void read_data(){
        size_t length = socket->read_some(boost::asio::buffer(header_buffer, sizeof(MessageHeader)));
        std::cout<<length<<std::endl;
    }

    void hendle_cicle(){
        greeting();

    while (true) {
        std::memset(header_buffer, 0, sizeof(header_buffer));

        try {
            size_t length = socket->read_some(boost::asio::buffer(header_buffer, sizeof(MessageHeader)));
            auto header = MessageHeader::deserialize(header_buffer);
            std::cout << header->size << std::endl;
            std::vector<char> data_buffer(header->size);
            size_t bytes_read = socket->read_some(boost::asio::buffer(data_buffer.data(), header->size));

            if (bytes_read != header->size) {
                std::cerr << "Error: incomplete data received, expected " << header->size << " bytes, got " << bytes_read << std::endl;
                continue;
            }

            switch (header->type) {
                case MessageType::GREETING: {
                    auto g = Greeting::deserialize(data_buffer.data());
                    std::cout << "Greeting received: " << g->msg << " " << g->uuid << std::endl;
                    break;
                }
                case MessageType::USERSLIST: {
                    auto cl = ClientsList::deserialize(data_buffer.data());
                    break;
                }
                case MessageType::ADD_TO_GROUP: {
                    auto r = SRoom::deserialize(data_buffer.data());
                    std::cout << r->room_name << std::endl;
                    std::cout << "joined uuid " << r->joined_uuid << std::endl;

                    if (strlen(r->room_name) == 0) {
                        boost::uuids::uuid uuid = boost::uuids::random_generator()();
                        auto s_uuid = boost::lexical_cast<std::string>(uuid);
                        room = std::make_shared<Room>();
                        room->setName(s_uuid);
                        room->addClient(shared_from_this());
                    }

                    if (strlen(r->joined_uuid) != 0) {
                        auto j_client = ClientStore::getInstance().findClient(r->joined_uuid);
                        room->addClient(j_client);
                        std::memset(r->joined_uuid, 0, sizeof(r->joined_uuid));
                    }

                    strcpy(r->room_name, room->getName().c_str());
                    std::memset(r->uuids, 0, sizeof(r->uuids));

                    int client_index = 0;
                    for (auto& client : room->getClients()) {
                        if (client) {
                            std::string client_uuid = boost::lexical_cast<std::string>(client->getUuid());
                            std::cout << client_uuid.c_str() << std::endl;
                            if (client_index < 5) {
                                strcpy(r->uuids[client_index], client_uuid.c_str());
                                client_index++;
                            }
                        }
                    }

                    send_message(MessageType::ADD_TO_GROUP, *r);
                    break;
                }

                default:
                    break;
            }

        } catch (const boost::system::system_error& e) {
            if (e.code() == boost::asio::error::eof) {
                std::cerr << "Client disconnected (EOF)" << std::endl;
                break;
            } else {
                std::cerr << "System error: " << e.what() << std::endl;
                break;
            }
        }
    }
    socket->close();
    }

    
};



int main() {
    try {
        io_context io;
        tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 12345));
        std::cout << "Server started on port 12345" << std::endl;

        while (true) {
            auto socket = std::make_shared<tcp::socket>(io);
            acceptor.accept(*socket);
            std::shared_ptr<Client> client = std::make_shared<Client>(socket);
        
            std::thread([client]() {
                client->authorise();
                client->hendle_cicle();
            }).detach();
           
        }
    } catch (std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
    }
    return 0;
}
