#include "server.h"

#include <memory>

#include "Networking/Networking.h"

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
        int client_size = sizeof(clientAddr);
        client_socket = accept(
            server_socket,
            reinterpret_cast<sockaddr*>(&clientAddr),
            &client_size);

        if (client_socket != INVALID_SOCKET)
        {
            std::cout << "Client connected: " << inet_ntoa(clientAddr.sin_addr) << "\n";


        trying_to_get_id:
            std::string buffer(1024, '\0');
            RecvData(client_socket, buffer, sizeof(buffer));

            size_t client_id = 0;
            json request = json::parse(buffer);

            if (request.contains("id") && request.at("id").is_number())
            {
                client_id = request.at("id");
            }
            else
            {
                std::cout << "Invalid id. Client must resend their id...\n";
                SendData(client_socket, ErrorMessageSendingClientIdS{
                             request.contains("id")
                                 ? ClientIdErrorIncorrect
                                 : ClientIdErrorType_Incorrect
                         });
                goto trying_to_get_id;
            }

            SendData(client_socket, ErrorMessageSendingClientIdS{ClientIdOk});


            // Try to find the client in the map
            if (client_threads.contains(client_id))
            {
                // If the client is already connected, close the prv connection
                if (client_threads[client_id].first.is_client_connected)
                {
                    std::cout << "Client already connected. Overeating client socket.\n";
                    closesocket(client_threads[client_id].first.client_socket);
                }

                // If the client is not connected, update the socket and the connection status
                client_threads[client_id].first.client_socket = client_socket;
                client_threads[client_id].first.is_client_connected = true;
                client_threads[client_id].second = std::thread(&Server::HandleClient, this, client_socket);
            }

            ClientThreadData client_thread_data;
            client_thread_data.id = client_id;
            client_thread_data.client_socket = client_socket;
            client_thread_data.is_client_connected = true;

            // Insert the client thread if new.
            std::thread client_thread_handle(&Server::HandleClient, this, client_socket);
            client_threads.insert(
                std::make_pair(client_id, std::make_pair(client_thread_data, std::move(client_thread_handle))));
        }
    }
}

void Server::EndServer()
{
    isRunning = false;


    for (auto& thread_data_pair : client_threads | std::views::values)
    {
        thread_data_pair.first.is_client_connected = false;
        thread_data_pair.second.join();
        closesocket(thread_data_pair.first.client_socket);
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

                // Select to send it to all clients or to a specific client.
                std::cout << "Enter client id or 'all' to send to all clients\n";
                for (auto& [_, snd] : client_threads)
                {
                    std::cout << "Client id: " << snd.first.id << "\n";
                }
                std::getline(std::cin, input);
                if (input == "all")
                {
                    for (auto& [_, snd] : client_threads)
                    {
                        switch (const ExecuteActionResult result = send_action_to_client(
                            actionIndex, snd.first.client_socket))
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
                        }
                    }
                }
                else
                {
                    const size_t client_id = std::stoull(input);
                    if (!client_threads.contains(client_id))
                    {
                        std::cout << "Client not found\n";
                        continue;
                    }

                    switch (const ExecuteActionResult result = send_action_to_client(
                            actionIndex, client_threads[client_id].first.client_socket))
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
                    }
                }
            }
        }

        catch (const std::invalid_argument&)
        {
            std::cout << "Invalid command\n";
        }
    }
}

void Server::PrintAllActionsWithIndex(const Actions& actions)
{
    for (size_t index = 0; index < actions.size(); ++index)
    {
        std::cout << index << ". " << actions[index]->getName() << "\n";
    }
}

Server::ExecuteActionResult Server::send_action_to_client(const int action_index, const SOCKET client_socket)
{
    if (action_index < 0 || action_index >= action_registry.client_actions.size())
    {
        std::cout << "Action not found\n";
        return ExecuteActionResult::ActionNotFound;
    }

    Request request;
    request.InitializeRequest(action_registry.client_actions[action_index]->getName(),
                              action_registry.client_actions[action_index]->serialize());

    int send_result = 0;
    int attempts = 0;
    do
    {
        send_result = send(client_socket, request.body.c_str(), static_cast<int>(request.body.size()), 0);
        if (send_result != SOCKET_ERROR)
        {
            break;
        }
        attempts++;
    }
    while (attempts < 10);

    if (send_result == SOCKET_ERROR)
    {
        // The client is not responding. Try again later.
        return ExecuteActionResult::ActionNotSent;
    }

    return ExecuteActionResult::ActionExecuted;
}
