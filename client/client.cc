#include <DataChannelServer/client/client.h>
#include <functional>
#include <iostream>
#include <string>

#include <json11/json11.hpp>

#include <html5.h>
#include <GLES2/gl2.h>

#include "shared/constants.h"


const char host[] = "localhost";

struct Client {
    int id;

    int x;
    int y;
};

std::map<int, Client> ParseClients(const json11::Json& clients) {
    std::map<int, Client> result;
    for (const auto& client_pair: clients.object_items()) {
        json11::Json::object client_obj = client_pair.second.object_items();
        Client client;
        client.id = client_obj["id"].int_value();
        client.x = client_obj["x"].int_value();
        client.y = client_obj["y"].int_value();

        result[client.id] = client;
    }

    return result;
}

std::string GetInfoLog(GLuint token) {
    int buffer_length;
    if (glIsProgram(token)) {
        glGetProgramiv(token, GL_INFO_LOG_LENGTH, &buffer_length);
    } else if (glIsShader(token)) {
        glGetShaderiv(token, GL_INFO_LOG_LENGTH, &buffer_length);
    } else {
        return "Unsupposed token type";
    }

    std::vector<char> buffer;
    buffer.resize(buffer_length);

    if (glIsProgram(token)) {
        glGetProgramInfoLog(token, buffer_length, NULL, buffer.data());
    } else if (glIsShader(token)) {
        glGetShaderInfoLog(token, buffer_length, NULL, buffer.data());
    } else {
        return "Unsupposed token type";
    }

    return std::string(std::begin(buffer), std::end(buffer));
}

const char* vertex_source = R"(
      attribute vec2 position;

      void main()
      {
        gl_Position = vec4(position, 0.0, 1.0);
      }
)";

const char* fragment_source = R"(
    void main()
    {
      gl_FragColor = vec4(1.0, 0, 0, 1.0);
    }
)";

void CreateMainProgram() {
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    GLuint program = glCreateProgram();

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_source, NULL);
    glCompileShader(vertex_shader);

    glAttachShader(program, vertex_shader);

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_source, NULL);
    glCompileShader(fragment_shader);

    glAttachShader(program, fragment_shader);

    glLinkProgram(program);
    glValidateProgram(program);

    int validate_status;
    glGetProgramiv(program, GL_VALIDATE_STATUS, &validate_status);

    if (!validate_status) {
        std::cout << "Bad program" << std::endl;

        std::cout << "Program Info Log: " << GetInfoLog(program) << std::endl;
        std::cout << "Vertex Info Log: " << GetInfoLog(vertex_shader) << std::endl;
        std::cout << "Fragment Info Log: " << GetInfoLog(fragment_shader) << std::endl;
    }

    glUseProgram(program);

    GLint position_location = glGetAttribLocation(program, "position");
    glEnableVertexAttribArray(position_location);
    glVertexAttribPointer(position_location, 2, GL_FLOAT, false, 4 * 2, 0);
}

class Game {
  enum GameState {
    WAITING_FOR_WELCOME,
    RUNNING_GAME
  };

  public:
    Game(std::shared_ptr<DataChannel> channel): channel_(channel) {
        channel->SetOnMessageHandler([this](const std::string& message) {
          HandleMessage(message);
        });
        channel->SetOnCloseHandler([]() {
            std::cout << "The socket is closed " << std::endl;
        });

        state = WAITING_FOR_WELCOME;

        EmscriptenWebGLContextAttributes attributes;
        emscripten_webgl_init_context_attributes(&attributes);

        EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context = emscripten_webgl_create_context("canvas", &attributes);

        auto error = emscripten_webgl_make_context_current(context);
        if (error != 0) {
            std::cout << "Got emscripten error" << error <<std::endl;
            abort();
        }

        error = emscripten_set_keydown_callback("canvas", this, false, [](int /*eventType*/, const EmscriptenKeyboardEvent *keyEvent, void *userData){
            Game* self = (Game*)(userData);

            std::string code(keyEvent->code);

            json11::Json::object press_key_message;
            press_key_message["type"] = "press_key";
            press_key_message["code"] = code;
            press_key_message["id"] = self->id;

            self->channel_->SendMessage(json11::Json(press_key_message).dump());

            return 1;
        });

        if (error != 0) {
            std::cout << "Got emscripten error" << error <<std::endl;
            abort();
        }

        CreateMainProgram();
    }

    void RenderAndUpdate() {
        glClearColor(0, 1.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        std::vector<float> coords;

        for (const auto& client_entry : clients_) {
            const Client& client = client_entry.second;

            double x = (client.x - 50.0) / 50.0;
            double y = (client.y - 50.0) / 50.0;

            double dx = 10.0/50.0;
            double dy = 10.0/50.0;

            coords.push_back(x);
            coords.push_back(y);

            coords.push_back(x);
            coords.push_back(y+dy);

            coords.push_back(x+dx);
            coords.push_back(y+dy);

            coords.push_back(x);
            coords.push_back(y);

            coords.push_back(x+dx);
            coords.push_back(y);

            coords.push_back(x+dx);
            coords.push_back(y+dy);
        }

        glBufferData(GL_ARRAY_BUFFER, coords.size() * 4, coords.data(), GL_STREAM_DRAW);

        glDrawArrays(GL_TRIANGLES, 0, coords.size()/2);
    }

    void HandleMessage(const std::string& message_str) {
        std::string err;
        json11::Json message = json11::Json::parse(message_str, err);

        if (!err.empty()) {
            std::cout << "Got error parsing message" << err << " " << message_str << std::endl;
        }

        std::string type = message["type"].string_value();

        switch (state) {
            case WAITING_FOR_WELCOME:
                if (type == "welcome") {
                    id = message["id"].int_value();
                    clients_ = ParseClients(message["clients"]);
                    state = RUNNING_GAME;
                } else {
                    std::cout << "Was not expecting non-welcome message while waiting" << std::endl;
                    abort();
                }
                break;

            case RUNNING_GAME:
                if (type == "update") {
                    clients_ = ParseClients(message["clients"]);
                } else {
                    std::cout << "Was non-update while playing the game" << std::endl;
                    abort();
                }
                break;

            default:
                std::cout << "Unhandled state!!: " << (int)(state);
        }
    }

  private:
    int id;
    std::map<int, Client> clients_;
    std::shared_ptr<DataChannel> channel_;
    GameState state;
};

int main() {
    Connect(
        std::string(host), port, [](std::shared_ptr<DataChannel> channel) {
            Game* game = new Game(channel);
            emscripten_set_main_loop_arg([](void* game_data){
                Game* game = (Game*)(game_data);
                game->RenderAndUpdate();
            }, game, 0, 0);
        }, [](const std::string& error) {
          std::cout << "Could not connect " << error << std::endl;
        });
}