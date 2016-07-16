#include <iostream>

#include <DataChannelServer/server/server.h>

#include "shared/constants.h"

#include "json11/json11.hpp"

struct Client {
  int id;
  std::shared_ptr<DataChannel> channel;

  int x;
  int y;
};

json11::Json::object GetClients(const std::map<int, Client>& clients) {
  json11::Json::object client_states;
  for (const auto& client_entry : clients) {
    const Client& client = client_entry.second;
    json11::Json::object client_state;
    client_state["id"] = client.id;
    client_state["x"] = client.x;
    client_state["y"] = client.y;

    client_states[std::to_string(client.id)] = client_state;
  }
  return client_states;
}

void SendUpdateWorldMessages(const std::map<int, Client>& clients) {
  json11::Json::object update_message;
  update_message["type"] = "update";
  update_message["clients"] = GetClients(clients);

  for (const auto& client_entry: clients) {
    client_entry.second.channel->SendMessage(json11::Json(update_message).dump());
  }
}

int main() {
  std::cout << "HELLO WORLD!" << std::endl;

  Server server(port);

  std::map<int, Client> clients;
  int next_id = 0;

  server.SetConnectionHandler(
      [&clients, &next_id](std::shared_ptr<DataChannel> channel) {
        int id = next_id++;
        std::cout << "I have recieved a new channel!" << channel << id << std::endl;
        Client client;
        client.id = id;
        client.channel = channel;
        client.x = id * 10 % 100;
        client.y = id * 10 % 100;

        clients[id] = client;

        json11::Json::object welcome_message;
        welcome_message["type"] = "welcome";
        welcome_message["id"] = id;
        welcome_message["clients"] = GetClients(clients);

        channel->SendMessage(json11::Json(welcome_message).dump());

        SendUpdateWorldMessages(clients);

        channel->SetOnMessageHandler(
            [&clients, id](const std::string& message_str) {
              std::string err;
              json11::Json message = json11::Json::parse(message_str, err);

              if (!err.empty()) {
                  std::cout << "Got error parsing message" << err << " " << message_str << std::endl;
              }

              std::string type = message["type"].string_value();

              if (type == "press_key") {
                  int id = message["id"].int_value();
                  std::string code = message["code"].string_value();

                  Client& client = clients[id];

                  if (code == "ArrowUp") {
                    client.y += 3;
                  } else if (code == "ArrowDown") {
                    client.y -= 3;
                  } else if (code == "ArrowLeft") {
                    client.x -= 3;
                  } else if (code == "ArrowRight") {
                    client.x += 3;
                  } else {
                    std::cout << "Bad code " << code << std::endl;
                  }

                  SendUpdateWorldMessages(clients);

              } else {
                  std::cout << "Was not expected not a key press" << std::endl;
                  abort();
              }

            });
        channel->SetOnCloseHandler([&clients, id]() {
          std::cout << "Closing " << id << std::endl;
          clients.erase(id);

          SendUpdateWorldMessages(clients);
        });


      });
  server.Start();
}