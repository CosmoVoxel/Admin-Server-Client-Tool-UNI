#include "server.h"

#include <memory>

Server::Server()
{
    std::cout << "Server initialized.\n";
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

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET)
    {
        WSACleanup();
        throw std::runtime_error("Socket creation failed.");
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Привязка ко всем доступным IP

    if (bind(server_socket, reinterpret_cast<sockaddr*>(&serverAddr),
             sizeof(serverAddr)) == SOCKET_ERROR)
    {
        closesocket(server_socket);
        WSACleanup();
        throw std::runtime_error("Bind failed.");
    }

    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR)
    {
        closesocket(server_socket);
        WSACleanup();
        throw std::runtime_error("Listen failed.");
    }

    std::cout << "Server listening on port " << PORT << "...\n";
    isRunning = true;

    std::thread admin_thread(&Server::AdminThread, this, this);

    while (isRunning)
    {
        client_socket = accept(
            server_socket,
            reinterpret_cast<sockaddr*>(&clientAddr),
            reinterpret_cast<socklen_t*>(&clientAddr));

        if (client_socket != INVALID_SOCKET)
        {
            std::cout << "Client connected: " << inet_ntoa(clientAddr.sin_addr) << "\n";

            ClientThreadData client_thread_data;
            client_thread_data.client_socket = client_socket;
            client_thread_data.is_client_connected = true;

            // Insert the client thread
            std::thread client_thread_handle(&Server::HandleClient, this, client_socket);
            client_threads.insert(std::make_pair(client_socket, std::make_pair(client_thread_data, std::move(client_thread_handle))));
        }
    }
}

void Server::EndServer()
{
    isRunning = false;


    for (auto& [client_socket, client_thread] : client_threads)
    {
        client_thread.first.is_client_connected = false;
        client_thread.second.join();
        closesocket(client_socket);
    }


    closesocket(server_socket);
    WSACleanup();
}

// -----==== Action Management On Server ====-----
void Server::HandleClient(const SOCKET clientSocket)
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

void Server::AdminThread(Server* server)
{
    std::string input;
    std::cout << "Admin commands:\n";

    // Print all type od actions
    PrintAllActionsWithIndex(action_registry.client_actions);

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
                const ExecuteActionResult result = send_action_to_client(actionIndex, client_socket);

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
