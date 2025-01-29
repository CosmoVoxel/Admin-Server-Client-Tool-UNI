#include "client.h"

#include <Actions/ActionStructures.h>
#include <SystemManager/OperatingSystemManager.h>

#include "RequestBuilder/RequestBuilder.h"

void Client::InitializeConnection() {
    GenerateId(true);
#ifdef _WIN32
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("WSAStartup failed.");
    }
#endif

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        WSACleanup();
        throw std::runtime_error("Socket creation failed.");
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
}

// ----------------------------------========Helper Functions========---------------------------------- //
// Helper: Parse JSON from received buffer
std::optional<json> Client::ParseJson(const std::string &buffer) {
    try {
        return json::parse(buffer);
    } catch (const json::parse_error &) {
        std::cerr << "Error parsing JSON response.\n";
        return std::nullopt;
    }
}

// Helper: Attempt to reconnect
bool Client::AttemptReconnect() {
    while (connect(server_socket, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed. Retrying...\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return true;
}

// Helper: Send client ID to the server.
bool Client::SendClientId() {
#ifdef _ADMIN
    // Send password and login to server with id
    Request request;
    request.InitializeRequest("AdminCredential", AdminCredentialS{}, &id);
    std::string buffer(1024, 0);
    switch (SendData(server_socket, request.body)) {
        case DataStatus::DataSent:
            std::cout << "Admin credentials sent to the server.\n";
            switch (RecvData(server_socket, buffer)) {
                case DataStatus::DataReceived: {
                    const auto errorType = ProcessServerResponse(buffer);
                    if (errorType.value() == Ok) {
                        return true;
                    }

                    std::cerr << "Incorrect Admin credentials. Reconnecting...\n";
                    return false;
                }
                case DataStatus::DataNotReceived:
                    std::cerr << "No data received. Reconnecting...\n";
                    return false;
                case DataStatus::UnknownReceivedError:
                    std::cerr << "Unknown error while receiving data. Reconnecting...\n";
                    return false;
                default:
                    std::cerr << "Unknown error while receiving data. Reconnecting...\n";
            };
            return true;

        case DataStatus::DataNotSent:
        case DataStatus::UnknownSentError:
            std::cerr << "Failed to send Admin credentials. Reconnecting...\n";
            return AttemptReconnect();
        default: break;
    };

#else
    std::cout << "Client auto gen id: " << id << "\n";

    std::string select_id;
    std::cout << "Change id? [y,n]: ";
    std::getline(std::cin, select_id);
    std::ranges::transform(select_id, select_id.begin(), [](const unsigned char c) { return std::tolower(c); });
    if (select_id == "y") {
        std::string id_str;
        std::cout << "Enter the ID: ";
        std::getline(std::cin, id_str);
        id = std::stoull(id_str);
    }

    const std::string id_message = "{\"id\":" + std::to_string(id) + "}";

    std::string buffer(1024, 0);
    switch (SendData(server_socket, id_message)) {
        case DataStatus::DataSent:
            std::cout << id << ":ID sent to the server.\n";
            switch (RecvData(server_socket, buffer)) {
                case DataStatus::DataReceived: {
                    const auto errorType = ProcessServerResponse(buffer);
                    if (errorType.value() == Ok) {
                        return true;
                    }

                    std::cerr << "Incorrect client ID. Reconnecting...\n";
                    return false;
                }
                case DataStatus::DataNotReceived:
                    std::cerr << "No data received. Reconnecting...\n";
                    return false;
                case DataStatus::UnknownReceivedError:
                    std::cerr << "Unknown error while receiving data. Reconnecting...\n";
                    return false;
                default: break;
            };

            return true;

        case DataStatus::DataNotSent:
        case DataStatus::UnknownSentError:
            std::cerr << "Failed to send ID. Reconnecting...\n";
            return AttemptReconnect();
        default: break;
    }
#endif

    return false;
}

// Helper: Process the server response
std::optional<ClientIdErrorType> Client::ProcessServerResponse(const std::string &buffer) {
    auto parsed_json = ParseJson(buffer);
    if (!parsed_json) {
        return std::nullopt;
    }

    try {
        int error_type = parsed_json->at("error_type").get<int>();
        return static_cast<ClientIdErrorType>(error_type);
    } catch (const json::out_of_range &) {
        std::cerr << "Invalid response from server.\n";
        return std::nullopt;
    }
}

// Helper: Handle server ID error response
void Client::HandleIdError(const ClientIdErrorType errorType) {
    switch (errorType) {
        case Incorrect:
            std::cerr << "Server indicated incorrect client ID. Regenerating ID...\n";
            GenerateId(true); // Assume this is a member function to regenerate the ID
            break;

        case Ok:
            std::cout << "Client ID is correct.\n";
            break;
    }
}

// Main connection function
void Client::TryToConnect() {
    bool is_info_send = false;

    while (!is_info_send) {
        AttemptReconnect();

        // Try to send the client ID
        if (SendClientId()) {
            is_info_send = true;
        }
    }

    bool is_connected = false;

    while (!is_connected) {
        std::string buffer(1024, '\0');

        switch (RecvData(server_socket, buffer)) {
            case DataStatus::DataReceived: {
                std::cout << "Received: " << buffer << "\n";
                const auto errorType = ProcessServerResponse(buffer);

                if (errorType) {
                    HandleIdError(errorType.value());
                }
                is_connected = true;
            }

            case DataStatus::DataNotReceived:
                std::cerr << "No data received. Retrying...\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                break;

            case DataStatus::UnknownReceivedError:
                std::cerr << "Unknown error while receiving data. Reconnecting...\n";
                if (!AttemptReconnect()) {
                    is_connected = true;
                }
                break;
            default: break;
        }
    }

    receiveThread = std::thread(&Client::WaitingForCommands, this);
}

// This function will be used for handling messages from the server...
void Client::WaitingForCommands() {
    while (true) {
        const int bytesReceived = recv(server_socket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived > 0) {
            std::string data(buffer, bytesReceived);
            std::cout << "Client Received: " << data << "\n";
            DoAction(data);
            memset(buffer, 0, BUFFER_SIZE);
        } else if (bytesReceived == SOCKET_ERROR) {
            std::cerr << "Connection lost.\n";
            TryToConnect();
        }
    }
}

void Client::DoAction(const std::string &data) {
    const nlohmann::json json_data = nlohmann::json::parse(data);

    if (json_data.empty()) {
        std::cerr << "Error: Invalid JSON data\n";
        return;
    }
    if (!json_data.contains("index")) {
        std::cerr << "Error: Invalid JSON data\n";
        return;
    }
    if (!json_data.contains("transaction_id")) {
        std::cerr << "Error: Invalid JSON data\n";
        return;
    }

    json result = actionManager.executeAction(json_data);

    // Add transaction_id and index to the result
    result["transaction_id"] = json_data.at("transaction_id");
    result["index"] = json_data.at("index");

    SendData(server_socket, result);
}

void Client::StopConnection() {
    closesocket(server_socket);
    WSACleanup();
    if (receiveThread.joinable()) {
        receiveThread.join();
    }
}

void Client::GenerateId(const bool add_random = true) {
    const std::string cpu_s = OperatingSystemManager::GetClientCPUSerial();
    const std::string motherboard_s = OperatingSystemManager::GetClientMotherboardSerial();
    const std::string hdd_s = OperatingSystemManager::GetClientHDDSerial();

    size_t rand_v = 0;
    if (add_random) {
        std::random_device rd;
        std::mt19937 gen(rd());
        rand_v = std::uniform_int_distribution<int>{0, 1023}(gen);
    }

    id = std::hash<std::string>{}(cpu_s + motherboard_s + hdd_s) + rand_v;
}
