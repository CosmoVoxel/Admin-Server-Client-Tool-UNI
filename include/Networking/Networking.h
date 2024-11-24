#pragma once
#include <vector>

// Windows only
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

// Get Data From Socket
inline std::string GetDataFromSocket(const SOCKET &clientSocket)
{
    int messageSize;
    recv(clientSocket, reinterpret_cast<char *>(&messageSize), sizeof(int), 0);

    // Get the message
    std::vector<char> message(messageSize);
    recv(clientSocket, message.data(), messageSize, 0);

    // Convert to string
    std::string messageStr(message.begin(), message.end());
    return messageStr;
}

// Send Data To Socket
inline void SendDataToSocket(const SOCKET &clientSocket, const std::string &message)
{
    int messageSize = message.size();
    send(clientSocket, reinterpret_cast<char *>(&messageSize), sizeof(int), 0);
    send(clientSocket, message.c_str(), messageSize, 0);
}
