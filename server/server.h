#pragma once
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

class Server {
public:
    Server();
    ~Server();

    WSADATA wsaData{};
    SOCKET serverSocket{}, clientSocket{};
    sockaddr_in serverAddr{}, clientAddr{};
    int PORT = 54000;
    bool isRunning = false;

    std::string admin_secret = "admin:admin";

    void StartServer();
    void EndServer();
    static void HandleClient(SOCKET clientSocket);

    std::vector<std::thread> clientThreads;
    std::thread adminThread;
    std::mutex clientThreadsMutex;
};

inline Server::Server() {
    std::cout << "Server initialized.\n";
}

inline Server::~Server() {
    EndServer();
    std::cout << "Server stopped.\n";
}

inline void Server::StartServer() {
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("WSAStartup failed.");
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        WSACleanup();
        throw std::runtime_error("Socket creation failed.");
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Привязка ко всем доступным IP

    if (bind(serverSocket, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(serverSocket);
        WSACleanup();
        throw std::runtime_error("Bind failed.");
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(serverSocket);
        WSACleanup();
        throw std::runtime_error("Listen failed.");
    }

    std::cout << "Server listening on port " << PORT << "...\n";
    isRunning = true;

    while (isRunning) {
        int clientSize = sizeof(clientAddr);
        clientSocket = accept(serverSocket, reinterpret_cast<sockaddr *>(&clientAddr), &clientSize);
        if (clientSocket != INVALID_SOCKET) {
            std::cout << "Client connected: " << inet_ntoa(clientAddr.sin_addr) << "\n";

            std::lock_guard<std::mutex> lock(clientThreadsMutex);
            clientThreads.emplace_back(HandleClient, clientSocket);
        }
    }
}

inline void Server::EndServer() {
    isRunning = false;

    {
        std::lock_guard<std::mutex> lock(clientThreadsMutex);
        for (auto &thread : clientThreads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

    closesocket(serverSocket);
    WSACleanup();
}

inline void Server::HandleClient(SOCKET clientSocket) {
    char buffer[1024];
    while (true) {
        ZeroMemory(buffer, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            std::cout << "Received: " << buffer << "\n";
            send(clientSocket, buffer, bytesReceived, 0);
        } else {
            break;
        }
    }

    closesocket(clientSocket);
    std::cout << "Client disconnected.\n";
}
