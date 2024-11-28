#pragma once
#include <thread>

#include <winsock2.h>
#include <ws2tcpip.h>

#include <Actions/ActionSystem.h>

#pragma comment(lib, "ws2_32.lib")


class Client {
public:
    ~Client() = default;

    static constexpr int PORT = 54000;
    static constexpr int BUFFER_SIZE = 1024;

    WSADATA wsaData{};
    sockaddr_in serverAddr{};
    char buffer[BUFFER_SIZE]{};
    SOCKET clientSocket{};
    std::thread receiveThread;
    std::thread thread_send;

    // Actions
    ActionFactory actionFactory{};
    ActionManager actionManager{actionFactory};


    void InitializeConnection();
    void TryToConnect();
    void WaitingForCommands();
    void DoAction(const std::string& data);
    void StopConnection();
};
