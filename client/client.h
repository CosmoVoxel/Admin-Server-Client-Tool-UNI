#pragma once
#include <iostream>
#include <thread>
#include <Actions/Actions.h>
#include <Networking/Networking.h>



class Client {
public:
    Client() = default;
    ~Client() = default;

    static constexpr int PORT = 54000;
    static constexpr int BUFFER_SIZE = 1024;

    WSADATA wsaData{};
    sockaddr_in serverAddr{};
    char buffer[BUFFER_SIZE]{};
    SOCKET clientSocket{};
    std::thread receiveThread;

    void InitializeConnection();
    void TryToConnect();
    void WaitingForCommands();
    void DoAction();
    void StopConnection();
};

inline void Client::InitializeConnection() {
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("WSAStartup failed.");
    }

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        WSACleanup();
        throw std::runtime_error("Socket creation failed.");
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr); // Явное задание IP
}

inline void Client::TryToConnect() {
    while (connect(clientSocket, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed. Retrying...\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << "Connected to the server!\n";

    receiveThread = std::thread(&Client::WaitingForCommands, this);
}

inline void Client::WaitingForCommands() {
    while (true) {
        const int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived > 0) {
            std::cout << "Server: " << std::string(buffer, bytesReceived) << "\n";
        } else if (bytesReceived == 0 || bytesReceived == SOCKET_ERROR) {
            std::cerr << "Connection lost.\n";
            break;
        }
    }
}

inline void Client::DoAction() {
}

inline void Client::StopConnection() {
    closesocket(clientSocket);
    WSACleanup();
    if (receiveThread.joinable()) {
        receiveThread.join();
    }
}
