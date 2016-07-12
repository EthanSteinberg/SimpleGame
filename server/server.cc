#include <iostream>

#include <DataChannelServer/server/server.h>

int main() {
    std::cout<< "HELLO WORLD!" << std::endl;
    Server server(9021);
    std::vector<std::shared_ptr<DataChannel>> channels;
    server.SetConnectionHandler([&channels](std::shared_ptr<DataChannel> channel) {
        std::cout<< "I have recieved a new channel!" << channel <<std::endl;
        channel->SendMessage("foo", 3);
        channel->SetMessageHandler([channel](const char* message, int message_length) {
            std::cout <<" Got message " << std::string(message, message_length) << std::endl;
            channel->SendMessage("SURE", 4);
        });
        channels.push_back(channel);
    });
    server.Start();
}