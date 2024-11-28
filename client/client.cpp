#include "client.h"

#include <Actions/ActionStructures.h>

void Client::InitializeConnection()
{
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        throw std::runtime_error("WSAStartup failed.");
    }

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET)
    {
        WSACleanup();
        throw std::runtime_error("Socket creation failed.");
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);
}

void Client::TryToConnect()
{
    while (connect(clientSocket, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "Connection failed. Retrying...\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << "Connected to the server!\n";

    receiveThread = std::thread(&Client::WaitingForCommands, this);
}

// This function will be used for handling messages from the server...
void Client::WaitingForCommands()
{
    while (true)
    {
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived > 0 && std::strlen(buffer) > 0)
        {
            std::string data(buffer, bytesReceived);
            std::cout << "Client Received: " << data << "\n";
            DoAction(data);
            ZeroMemory(buffer, BUFFER_SIZE);
            bytesReceived = 0;
        }

        else if (bytesReceived == SOCKET_ERROR)
        {
            std::cerr << "Connection lost.\n";
            break;
        }
    }
}


void Client::DoAction(const std::string &data)
{
    const nlohmann::json json_data = nlohmann::json::parse(data);

    if (json_data.empty())
    {
        std::cerr << "Error: Invalid JSON data\n";
        return;
    }
    if (!json_data.contains("index"))
    {
        std::cerr << "Error: Invalid JSON data\n";
        return;
    }
    if (!json_data.contains("transaction_id"))
    {
        std::cerr << "Error: Invalid JSON data\n";
        return;
    }

    json result = actionManager.executeAction(json_data);

    // Add transaction_id and index to the result
    result["transaction_id"] = json_data.at("transaction_id");
    result["index"] = json_data.at("index");


    std::string message;
    if (result.empty())
    {
        message = "Error: Invalid action";
    }
    else
    {
        // Convert result to string
        message = result.dump();
    }

    send(clientSocket, message.c_str(), message.size(), 0);
}

void Client::StopConnection()
{
    closesocket(clientSocket);
    WSACleanup();
    if (receiveThread.joinable())
    {
        receiveThread.join();
    }
}