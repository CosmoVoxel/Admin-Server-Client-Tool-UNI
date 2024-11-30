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
                client_threads[client_id].second = std::thread(&Server::HandleClient, this);
            }

            ClientThreadData client_thread_data;
            client_thread_data.id = client_id;
            client_thread_data.client_socket = client_socket;
            client_thread_data.is_client_connected = true;

            // Insert the client thread if new.
            std::thread client_thread_handle(&Server::HandleClient, this);
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
void Server::HandleClient()
{
    while (true)
    {
        std::cout << "Updating client status...\n";
        std::cout << "Blocking admin thread from parsing the client status...\n";
        std::lock_guard lock(admin_thread_mutex);
        for (auto& [id,thread_data_pair] : client_threads)
        {
            for (const auto& action : action_registry.status_update_actions)
            {
                std::string buffer(1024, '\0');
                Request request;
                json response;
                request.InitializeRequest(action->getName(), action->serialize());
                switch (SendData(thread_data_pair.first.client_socket, request.body, request.body.size()))
                {
                case DataSent:
                    std::cout << "Request sent to client with id: " << id << "\n";
                    break;
                case DataNotSent:
                    std::cout << "Request not sent to client with id: " << id << "\n";
                    break;
                case UnknownSentError:
                    std::cout << "Unknown error while sending request to client with id: " << id << "\n";
                    break;
                };
                switch (RecvData(thread_data_pair.first.client_socket, buffer, 1024))
                {
                case DataReceived:
                    std::cout << "Received: " << buffer << "\n";
                    response = json::parse(buffer);
                    switch (Request::CompareRequests(request, response))
                    {
                    case Request::Ok:
                        std::cout << "PC status is up\n";
                        client_threads[id].first.is_pc_up = true;
                        client_threads[id].first.update_status_time();
                        break;
                    case !Request::Ok:
                        std::cout << "PC status is down\n";
                        client_threads[id].first.is_pc_up = false;
                        client_threads[id].first.update_status_time();
                        break;
                    default: break;
                    };
                    break;
                case DataNotReceived:
                    std::cout << "Data not received\n";
                    break;
                case UnknownReceivedError:
                    std::cout << "Unknown error while receiving data\n";
                    break;
                };
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(30));
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
                if (actionIndex < 0 || actionIndex >= action_registry.client_actions.size())
                {
                    std::cout << "Action not found\n";
                    continue;
                }
                auto action = action_registry.client_actions[actionIndex];

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
                        // Send the action to the client
                        Request request;
                        json response;
                        std::string buffer(1024, '\0');
                        request.InitializeRequest(action->getName(), action->serialize());
                        switch (SendData(snd.first.client_socket, request.body, request.body.size()))
                        {
                        case DataSent:
                            std::cout << "Request sent to client with id: " << snd.first.id << "\n";
                        // Wait for the response
                            switch (RecvData(snd.first.client_socket, buffer, 1024))
                            {
                            case DataReceived:
                                response = json::parse(buffer);
                                switch (Request::CompareRequests(request, response))
                                {
                                case Request::Ok:
                                    std::cout << "Received: " << response << "\n";
                                    break;
                                default: break;
                                }
                                break;
                            case !DataReceived:
                                std::cout << "Request not sent to client with id: " << snd.first.id << "\n";
                                break;
                            default: break;
                            }
                            break;
                        case !DataSent:
                            std::cout << "Request not sent to client with id: " << snd.first.id << "\n";
                        default: break;
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
                    if (!client_threads[client_id].first.is_client_connected)
                    {
                        std::cout << "Client is not connected\n";
                        continue;
                    }
                    if (!client_threads[client_id].first.is_pc_up)
                    {
                        std::cout << "Client PC is down\n";
                        continue;
                    }
                    auto [fst, _] = std::move(client_threads[client_id]);
                    Request request;
                    json response;
                    std::string buffer(1024, '\0');
                    request.InitializeRequest(action->getName(), action->serialize());
                    switch (SendData(fst.client_socket, request.body, request.body.size()))
                    {
                    case DataSent:
                        std::cout << "Request sent to client with id: " << fst.id << "\n";
                    // Wait for the response
                        switch (RecvData(fst.client_socket, buffer, 1024))
                        {
                        case DataReceived:
                            response = json::parse(buffer);
                            switch (Request::CompareRequests(request, response))
                            {
                            case Request::Ok:
                                std::cout << "Rcv data \n";
                                if (response.contains("data"))
                                {
                                    std::cout << response.at("data").dump(4) << "\n";
                                }
                                break;
                            default: break;
                            }
                            break;
                        case !DataReceived:
                            std::cout << "Request not sent to client with id: " << fst.id << "\n";
                            break;
                        default: break;
                        }
                        break;
                    case !DataSent:
                        std::cout << "Request not sent to client with id: " << fst.id << "\n";
                    default: break;
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
