#include "server.h"


// -----------------============NETWORKING============----------------- //
Server::Server() {
    std::cout << "Server initialized.\n";
}

Server::~Server() {
    EndServer();
    std::cout << "Server stopped.\n";
}

void Server::StartServer() {
#if _WIN32
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("WSAStartup failed.");
    }
#endif


    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        WSACleanup();
        throw std::runtime_error("Socket creation failed.");
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Привязка ко всем доступным IP

    if (bind(server_socket, reinterpret_cast<sockaddr *>(&serverAddr),
             sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(server_socket);
        WSACleanup();
        throw std::runtime_error("Bind failed.");
    }

    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(server_socket);
        WSACleanup();
        throw std::runtime_error("Listen failed.");
    }

    std::cout << "Server listening on port " << PORT << "...\n";
    isRunning = true;

    std::thread admin_thread(&Server::AdminThread, this, this);

    sockaddr_in clientAddr{};
    SOCKET client_socket{};

    while (isRunning) {
        socklen_t client_size = sizeof(clientAddr);
        client_socket = accept(
            server_socket,
            reinterpret_cast<sockaddr *>(&clientAddr),
            &client_size);

        if (client_socket != INVALID_SOCKET) {
            std::cout << "Client connected: " << inet_ntoa(clientAddr.sin_addr) << "\n";

            size_t client_id = 0;
            bool is_admin = false;

            // Receiving client ID
            while (true) {
                std::string buffer(1024, '\0');
                RecvData(client_socket, buffer);

                try {
                    json request = json::parse(buffer);

                    // Check if the client is an admin
                    if (request.contains("index") &&
                        request.contains("data") &&
                        request.at("index").is_string() &&
                        request.at("index") == "AdminCredential") {
                        if (request.contains("data")) {
                            if (request.at("data") == AdminCredentialS{}) {
                                client_id = request.at("id");
                                is_admin = true;
                                SendData(client_socket, ErrorMessageSendingClientIdS{Ok});
                            } else {
                                SendData(client_socket, ErrorMessageSendingClientIdS{Incorrect});
                                throw std::invalid_argument("Invalid Admin credentials...");
                            }
                        }
                    } else {
                        if (request.contains("id") && request.at("id").is_number()) {
                            client_id = request.at("id");
                            SendData(client_socket, ErrorMessageSendingClientIdS{Ok});
                            break;
                        } else {
                            SendData(client_socket, ErrorMessageSendingClientIdS{Incorrect});
                            throw std::invalid_argument("Invalid ID");
                        }
                    }
                } catch
                (const std::exception &e) {
                    std::cout << "Invalid ID. Client must resend their ID...\n";
                    std::cerr << "Error: " << e.what() << "\n";
                    SendData(client_socket, ErrorMessageSendingClientIdS{Incorrect});
                }
            }
            // Check if the client is already connected
            if (client_threads.contains(client_id)) {
                auto &client_data = client_threads[client_id].first;
                if (client_data->is_client_connected) {
                    std::cout << "Client already connected. Closing previous connection.\n";
                    std::cout << "Old client socket: " << client_data->client_socket << "\n";
                    std::cout << "New client socket: " << client_socket << "\n";
                    closesocket(client_data->client_socket);
                }
                client_data->client_socket = client_socket;
                if (is_admin) {
                    client_data->is_admin = true;
                }
                client_data->is_client_connected = true;
            } else {
                // Add new client
                auto client_thread_data = std::make_unique<ClientThreadData>();
                client_thread_data->id = client_id;
                client_thread_data->client_socket = client_socket;
                client_thread_data->is_client_connected = true;

                client_threads[client_id] = std::make_pair(
                    std::move(client_thread_data),
                    std::thread(&Server::HandleClient, this));
            }
        }
    }
}

void Server::EndServer() {
    isRunning = false;


    for (auto &[fst, snd]: client_threads | std::views::values) {
        fst->is_client_connected = false;
        snd.join();
        closesocket(fst->client_socket);
    }


    closesocket(server_socket);
    WSACleanup();
}


// -----------------============HELPERS============----------------- //
std::optional<json> Server::ReceiveAndParseResponse(const int client_socket, std::string &buffer) {
    switch (RecvData(client_socket, buffer)) {
        case DataStatus::DataReceived:
            try {
                return json::parse(buffer);
            } catch (const json::parse_error &) {
                std::cout << "JSON parse error\n";
                return std::nullopt;
            }

        case DataStatus::DataNotReceived:
            std::cout << "Data not received\n";
            return std::nullopt;

        case DataStatus::UnknownReceivedError:
        default:
            std::cout << "Unknown error while receiving data\n";
            return std::nullopt;
    }
}

void Server::ProcessClientAction(const size_t id, ClientThreadData *thread_data,
                                 const Request &request,
                                 const json &action_data) {
    // Prepare buffer and send data
    std::string buffer(1024, '\0');
    switch (SendData(thread_data->client_socket, request.body)) {
        case DataStatus::DataSent:
            std::cerr << "Request sent to client with id: " << id << "\n";
            break;
        case DataStatus::DataNotSent:
        case DataStatus::UnknownSentError:
            std::cerr << "Error sending request to client with id: " << id << "\n";
            thread_data->is_client_connected = false;
            thread_data->update_status_time();
            return;
        default: break;
    }

    // Receive and process response
    const auto response_opt = ReceiveAndParseResponse(thread_data->client_socket, buffer);
    if (!response_opt) {
        std::cerr << "Failed to receive valid response from client with id: " << id << "\n";
        thread_data->is_client_connected = false;
        thread_data->update_status_time();
        return;
    }

    const json &response = response_opt.value();
    if (Request::CompareRequests(request, response)) {
        //std::cout << "PC status is up for client id: " << id << "\n";
        thread_data->is_client_connected = true;
    } else {
        //std::cout << "PC status is down for client id: " << id << "\n";
        thread_data->is_client_connected = true;
    }
    thread_data->update_status_time();
}

void Server::HandleClientAction(const size_t client_id, const Request &request) {
    auto &[thread_data, _] = client_threads.at(client_id);

    if (!thread_data->is_client_connected) {
        std::cout << "Client is not connected\n";
        return;
    }

    std::string buffer(1024, '\0');
    switch (SendData(thread_data->client_socket, request.body)) {
        case DataStatus::DataSent:
            std::cout << "Request sent to client with id: " << thread_data->id << "\n";
            break;

        default:
            std::cout << "Failed to send request to client with id: " << thread_data->id << "\n";
            return;
    }

    auto response_opt = ReceiveAndParseResponse(thread_data->client_socket, buffer);
    if (response_opt && Request::CompareRequests(request, response_opt.value())) {
        std::cout << "Received valid response: " << response_opt.value() << "\n";
    } else {
        std::cout << "Invalid or no response from client with id: " << thread_data->id << "\n";
    }
}

void Server::BroadcastAction(const Request &request, const json &action_data) {
    for (auto &thread_pair: client_threads | std::views::values) {
        HandleClientAction(thread_pair.first->id, request);
    }
}

void Server::PrintAllActionsWithIndex(const Actions &actions) {
    for (size_t index = 0; index < actions.size(); ++index) {
        std::cout << index << ". " << actions[index]->getName() << "\n";
    }
}


// ---------------------=============Action Management On Server=============--------------------- //
void Server::HandleClient() {
    while (true) {
        std::cout << "Updating client status...\n";

        std::lock_guard lock(admin_thread_mutex);

        for (auto &[id, thread_data_pair]: client_threads) {
            auto data_ptr = thread_data_pair.first.get();
            if (!data_ptr->is_client_connected) {
                // std::cout << "Client with id :" << id << " is not connected\n";
                continue;
            }
            for (const auto &action: action_registry.status_update_actions) {
                Request request;
                request.InitializeRequest(action->getName(), action->serialize());
                ProcessClientAction(id, data_ptr, request, action->serialize());
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void Server::AdminThread(Server *server) {
    std::string input;
    std::cout << "Admin commands:\n";
    PrintAllActionsWithIndex(action_registry.client_actions);

    while (true) {
        std::cout << "Enter action index or 'exit' to stop the server\n";
        std::getline(std::cin, input);

        if (input == "exit") {
            std::cout << "Are you sure you want to stop the server? (y/n)\n";
            std::getline(std::cin, input);
            if (input == "y") {
                server->EndServer();
                break;
            }
            continue;
        }

        if (!input.empty() && std::ranges::all_of(input, isdigit)) {
            int actionIndex = std::stoi(input);
            if (actionIndex < 0 || actionIndex >= action_registry.client_actions.size()) {
                std::cout << "Action not found\n";
                continue;
            }

            auto action = action_registry.client_actions[actionIndex];
            Request request;
            request.InitializeRequest(action->getName(), action->serialize());

            std::cout << "Enter client id or 'all' to send to all clients\n";
            for (const auto &thread_pair: client_threads | std::views::values | std::views::keys) {
                std::cout << "Client id: " << thread_pair->id << "\n";
            }
            std::getline(std::cin, input);

            if (input == "all") {
                BroadcastAction(request, action->serialize());
            } else {
                try {
                    const size_t client_id = std::stoull(input);
                    if (!client_threads.contains(client_id)) {
                        std::cout << "Client not found\n";
                        continue;
                    }
                    if (!client_threads[client_id].first->is_client_connected) {
                        std::cout << "Client is not connected\n";
                        continue;
                    }
                    HandleClientAction(client_id, request);
                } catch (const std::invalid_argument &) {
                    std::cout << "Invalid client id\n";
                }
            }
        } else {
            std::cout << "Invalid command\n";
        }
    }
}
