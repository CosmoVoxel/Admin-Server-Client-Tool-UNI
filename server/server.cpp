#include "server.h"

#include <memory>

Server::Server()
{
    std::cout << "Server initialized.\n";
    RegisterActions();
}

Server::~Server()
{
    EndServer();
    std::cout << "Server stopped.\n";
}

void Server::StartServer()
{
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        throw std::runtime_error("WSAStartup failed.");
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET)
    {
        WSACleanup();
        throw std::runtime_error("Socket creation failed.");
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Привязка ко всем доступным IP

    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr),
             sizeof(serverAddr)) == SOCKET_ERROR)
    {
        closesocket(serverSocket);
        WSACleanup();
        throw std::runtime_error("Bind failed.");
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        closesocket(serverSocket);
        WSACleanup();
        throw std::runtime_error("Listen failed.");
    }

    std::cout << "Server listening on port " << PORT << "...\n";
    isRunning = true;

    std::thread adminThread(&Server::AdminThread, this, this);

    while (isRunning)
    {
        int clientSize = sizeof(clientAddr);
        clientSocket = accept(
            serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientSize);
        if (clientSocket != INVALID_SOCKET)
        {
            std::cout << "Client connected: " << inet_ntoa(clientAddr.sin_addr)
                << "\n";

            std::lock_guard lock(clientThreadsMutex);

            std::thread clientThreadHandle(&Server::HandleClient, this, clientSocket);
            std::thread clientThreadUpdate(&Server::UpdateClientStatus, this, clientSocket);

            clientThreads[clientSocket].emplace_back(std::move(clientThreadHandle));
            clientThreads[clientSocket].emplace_back(std::move(clientThreadUpdate));
        }
    }
}

void Server::EndServer()
{
    isRunning = false;

    {
        std::lock_guard lock(clientThreadsMutex);
        for (auto& client_threads : clientThreads | std::views::values)
        {
            for (auto& thread : client_threads)
            {
                if (thread.joinable())
                {
                    thread.join();
                }
            }
        }
    }

    closesocket(serverSocket);
    WSACleanup();
}

// -----==== Action Management On Server ====-----


void Server::HandleClient(SOCKET clientSocket)
{
    while (true)
    {
        char buffer[1024];
        const int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0)
        {
            std::string i(buffer, bytesReceived);
            // Remove the /n /r
            i.erase(std::ranges::remove_if(i, [](const char c) { return c == '\n' || c == '\r'; }).begin(), i.end());

            std::cout << "Received: " << i << "\n";

            if (json request = json::parse(i); request.contains("result"))
            {
                std::cout << "Result:" << request.at("result") << "\n";
            }
        }
    }
}

void Server::UpdateClientStatus(const SOCKET clientSocket)
{
    while (true)
    {
        Request request;
        // Send actions to the client
        for (const auto& action : debug_actions)
        {
            request.InitializeRequest(action->getName(), json{});
            send(clientSocket, request.body.c_str(), request.body.size(), 0);
        }

        int bytes_received = 0;
        char buffer[1024];

        auto start = std::chrono::steady_clock::now();

        while (true)
        {
            bytes_received = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytes_received > 0)
            {
                std::cout << "Received: " << buffer << "\n";

                // Parse the response
                json response = json::parse(buffer);
                std::string action_name;

                if (response.at("transaction_id") != request.transaction_id)
                {
                    std::cout << "Transaction id mismatch. Waiting for the correct response\n";
                    ZeroMemory(buffer, sizeof(buffer));
                    continue;
                }
                if (response.contains("index"))
                {
                    action_name = response.at("index");
                }
                else
                {
                    std::cout << "No action name found in the response\n";
                    ZeroMemory(buffer, sizeof(buffer));
                    continue;
                }

                // Find the action by name
                auto action_it = std::ranges::find_if(debug_actions, [&action_name](const auto& act)
                {
                    return act->getName() == action_name;
                });

                if (action_it == debug_actions.end())
                {
                    std::cout << "Action not found\n";
                    ZeroMemory(buffer, sizeof(buffer));
                    continue;
                }


                auto data = std::any_cast<PCStatus_S_OUT>(action_it->get()->deserialize(response.at("data")));


                std::cout << "Client status: " << data.ip << " " << data.mac << " " << data.os << "\n";
                clientStatuses[clientSocket] = data;
                ZeroMemory(buffer, sizeof(buffer));
                break;
            }
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - start).count() >= 30)
            {
                std::cout << "Client did not respond in 30 seconds. Waiting next update\n";
                ZeroMemory(buffer, sizeof(buffer));
                break;
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(60)); // 15 minutes
    }
}

void Server::AdminThread(Server* server)
{
    std::string input;
    std::cout << "Admin commands:\n";

    // Print all type od actions
    PrintAllActionsWithIndex(client_actions);

    while (true)
    {
        std::cout << "Enter action index or 'exit' to stop the server\n";
        std::getline(std::cin, input);
        // EXIT
        if (input == "exit")
        {
            std::cout << "Are you sure you want to stop the server? (y/n)\n";
            std::getline(std::cin, input);
            if (input == "y")
            {
                server->EndServer();
                break;
            }
        }

        try
        {
            if (!input.empty() && std::ranges::all_of(input, isdigit))
            {
                const int actionIndex = std::stoi(input);
                const ExecuteActionResult result = send_action_to_client(actionIndex,);

                switch (result)
                {
                case ActionExecuted:
                    std::cout << "Action executed\n";
                    break;
                case ActionNotFound:
                    std::cout << "Action not found\n";
                    break;
                case ActionNotSent:
                    std::cout << "Action not sent\n";
                    break;
                default:
                    break;
                }
            }
        }

        catch (const std::invalid_argument&)
        {
            std::cout << "Invalid command\n";
        }
    }
}
