#include <DataChannelServer/client/client.h>
#include <functional>
#include <iostream>
#include <string>

#include "shared/constants.h"

int main() {
  Connect(
      std::string("localhost"), port, [](std::shared_ptr<DataChannel> channel) {
        channel->SetMessageHandler([](const char *message, int message_length) {
          std::string thingy(message, message_length);
          std::cout << "I got " << thingy << std::endl;
        });
      });
}