#include "client.h"

#include <Actions/ActionStructures.h>

#include <SystemManager/OperatingSystemManager.h>

void Client::InitializeConnection()
{

    GenerateId(true);
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
    while (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "Connection failed. Retrying...\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Send id to server
    const std::string id_message = "{\"id\":" + std::to_string(id) + "}";
    std::cout << "Client id: " << id << "\n";
    switch (SendData(clientSocket, id_message))
    {
    case DataSent:
        std::cout << "ID sent to the server.\n";
        break;
    case DataNotSent:
        TryToConnect();
        break;
    case UnknownSentError:
        TryToConnect();
        break;
    };

try_to_receive:
    std::string buffer(BUFFER_SIZE, '\0');
    json j;
    switch (RecvData(clientSocket, buffer, BUFFER_SIZE))
    {
    case DataReceived:
        std::cout << "Received: " << buffer << "\n";
        j = json::parse(buffer);
        switch (static_cast<ClientIdErrorType>(j.at("error_type").get<int>()))
        {
        case ClientIdErrorType_Incorrect:
            GenerateId(true);
            TryToConnect();
            break;
        case ClientIdErrorIncorrect:
            std::cerr << "Error: Incorrect client id\n";
            break;
        case ClientIdOk:
            std::cout << "Client id is correct\n";
            break;
        }
        break;
    case DataNotReceived:
        goto try_to_receive;

    case UnknownReceivedError:
        TryToConnect();
        break;
    };


    receiveThread = std::thread(&Client::WaitingForCommands, this);
}

// This function will be used for handling messages from the server...
void Client::WaitingForCommands()
{
    while (true)
    {
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived > 0)
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
            TryToConnect();
        }
    }
}


void Client::DoAction(const std::string& data)
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

void Client::GenerateId(const bool add_random = true)
{
    const std::string cpu_s = OperatingSystemManager::GetClientCPUSerial();
    const std::string motherboard_s = OperatingSystemManager::GetClientMotherboardSerial();
    const std::string hdd_s = OperatingSystemManager::GetClientHDDSerial();

    size_t rand_v = 0;
    if (add_random)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        rand_v = std::uniform_int_distribution<int>{0, 1023}(gen);
    }

    id = std::hash<std::string>{}(cpu_s + motherboard_s + hdd_s) + rand_v;
}
