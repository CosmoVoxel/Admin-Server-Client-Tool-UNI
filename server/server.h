#pragma once
#include <iostream>
#include <map>
#include <mutex>
#include <queue>
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
    SOCKET server_socket{}, client_socket{};
    sockaddr_in serverAddr{}, clientAddr{};
    int PORT = 54000;
    bool isRunning = false;

    std::string admin_secret = "admin:admin";

    void StartServer();
    void EndServer();
    void HandleClient();
    void AdminThread(Server* server);


    ActionFactory actionFactory;

protected:
    std::thread adminThread;
    std::mutex admin_thread_mutex;

    struct ClientThreadData
    {
        size_t id{};
        SOCKET client_socket{};

        PCStatus_S_OUT status;
        bool is_client_connected{};
        bool is_pc_up{};
        int last_status_update_time{}; // UNIX timestamp

        std::queue<int> action_queue;

        void update_status_time()
        {
            last_status_update_time = static_cast<int>(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
        }

    };
    std::map<size_t,std::pair<ClientThreadData, std::thread>> client_threads;


    using Actions = std::vector<std::shared_ptr<Action>>;
    static void PrintAllActionsWithIndex(const Actions& actions);;
};