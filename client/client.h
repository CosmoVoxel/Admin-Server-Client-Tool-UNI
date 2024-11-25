#pragma once
#include <iostream>
#include <thread>
#include <Networking/Networking.h>
#include <Actions/Actions.cpp>

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
    void StartDefaultActions();
    void WaitingForCommands();
    static void SendingData();
    void DoAction(const std::string& data);
    void StopConnection();
};
