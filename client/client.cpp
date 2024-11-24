#include "client.h"

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
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr); // Явное задание IP

    for (const auto& actionCreator : actionFactory.actionRegistry | std::views::values)
    {
        const auto action = actionCreator();
        std::cout << action->getName() << "\n";
    }
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

void Client::WaitingForCommands()
{
    while (true)
    {
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived > 0)
        {
            std::string data(buffer, bytesReceived);
            DoAction(data);
        }
        else if (bytesReceived == SOCKET_ERROR)
        {
            std::cerr << "Connection lost.\n";
            break;
        }
    }
}

void Client::DoAction(const std::string &data) const
{
    const nlohmann::json jsonData = nlohmann::json::parse(data);
    actionManager.executeActions(jsonData);
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