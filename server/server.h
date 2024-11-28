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
    void HandleClient(SOCKET clientSocket);
    void UpdateClientStatus(SOCKET clientSocket);
    void AdminThread(Server* server);


    ActionFactory actionFactory;

protected:
    std::thread adminThread;

    struct ClientThreadData
    {
        int id;
        SOCKET client_socket;

        PCStatus_S_OUT status;
        bool is_client_connected;
        int last_status_update_time; // UNIX timestamp

        std::queue<int> action_queue;

        void update_status_time()
        {
            last_status_update_time = static_cast<int>(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
        }

    };
    std::map<SOCKET,std::pair<ClientThreadData, std::thread>> client_threads;


    using Actions = std::vector<std::shared_ptr<Action>>;
    static void PrintAllActionsWithIndex(const Actions& actions)
    {
        for (size_t index = 0; index < actions.size(); ++index)
        {
            std::cout << index << ". " << actions[index]->getName() << "\n";
        }
    };

    enum ExecuteActionResult
    {
        ActionNotFound = -1,
        ActionExecuted = 0,
        ActionNotSent = 1
    };
    ExecuteActionResult send_action_to_client(const int action_index, const SOCKET client_socket)
    {
        if (action_index < 0 || action_index >= action_registry.client_actions.size())
        {
            std::cout << "Action not found\n";
            return ExecuteActionResult::ActionNotFound;
        }

        Request request;
        request.InitializeRequest(action_registry.client_actions[action_index]->getName(), action_registry.client_actions[action_index]->serialize());

        int send_result = 0;
        int attempts = 0;
        do
        {
            if (attempts >= 10)
            {
                // Exit the loop. The client is not responding. Try again later.
                return ExecuteActionResult::ActionNotSent;
            }
            send_result = send(client_socket, request.body.c_str(), static_cast<int>(request.body.size()), 0);
            attempts++;
        }
        while (send_result == SOCKET_ERROR);

        client_threads[client_socket].first.action_queue.push(action_index);
        return ExecuteActionResult::ActionExecuted;
    };
};