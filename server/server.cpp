#include "server.h"

Server::Server() { std::cout << "Server initialized.\n"; }

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
            clientThreads.emplace_back(HandleClient, clientSocket);
        }
    }
}

void Server::EndServer()
{
    isRunning = false;

    {
        std::lock_guard lock(clientThreadsMutex);
        for (auto& thread : clientThreads)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }
    }

    closesocket(serverSocket);
    WSACleanup();
}

void Server::HandleClient(SOCKET clientSocket)
{
    while (true)
    {
        char buffer[1024];
        if (const int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0); bytesReceived > 0)
        {
            std::cout << "Received: " << buffer << "\n";
            send(clientSocket, buffer, bytesReceived, 0);
        }
        else
        {
            break;
        }
    }

    closesocket(clientSocket);
    std::cout << "Client disconnected.\n";
}

void Server::AdminThread(Server* server) const
{
    std::string input;
    std::cout << "Admin commands:\n";
    std::cout << "exit - stop the server\n";

    while (true)
    {
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
        else
        {
            // Send command to all clients
            send(clientSocket, input.c_str(), input.size(), 0);
        }
    }
}
