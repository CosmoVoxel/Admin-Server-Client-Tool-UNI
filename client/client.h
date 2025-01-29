#pragma once
#include <thread>
#include <random>

#include <Actions/ActionSystem.h>
#include <Networking/Networking.h>


class Client {
public:
    ~Client() = default;

    static constexpr int PORT = 54000;
    static constexpr int BUFFER_SIZE = 1024;

#if _WIN32
    WSADATA wsaData{};
#endif

    sockaddr_in server_addr{};
    SOCKET server_socket{};

    char buffer[BUFFER_SIZE]{};

    std::thread receiveThread;
    std::thread thread_send;

    // Actions
    ActionFactory actionFactory{};
    ActionManager actionManager{actionFactory};

    size_t id = 0;

    void InitializeConnection();

    std::optional<json> ParseJson(const std::string &buffer);

    bool AttemptReconnect();

    bool SendClientId();

    std::optional<ClientIdErrorType> ProcessServerResponse(const std::string &buffer);

    void HandleIdError(ClientIdErrorType errorType);

    void TryToConnect();

    void WaitingForCommands();

    void DoAction(const std::string &data);

    void StopConnection();

    void GenerateId(bool add_random);
};
