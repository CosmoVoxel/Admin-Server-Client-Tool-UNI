#pragma once
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

class Server
{
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

    void AdminThread(Server* server) const;

protected:
    std::vector<std::thread> clientThreads;
    std::thread adminThread;
    std::mutex clientThreadsMutex;
};
