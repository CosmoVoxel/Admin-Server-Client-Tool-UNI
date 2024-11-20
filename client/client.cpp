// #include <winsock2.h>
// #include <ws2tcpip.h>
// #include <iostream>
// #include <fstream>
//
// #pragma comment(lib, "Ws2_32.lib")
//
// #define SERVER_IP "127.0.0.1"
// #define PORT 54000
// #define BUFFER_SIZE 1024
// #define RANDOM_INT_OFFSET 100
// #define RANDOM_MAX_DEPTH 100
//
// int main() {
//     WSADATA wsaData;
//     sockaddr_in serverAddr{};
//     char buffer[BUFFER_SIZE];
//
//     // Initialize Winsock
//     if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
//         std::cerr << "WSAStartup failed.\n";
//         return 1;
//     }
//
//     // Create client socket
//     const SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
//     if (clientSocket == INVALID_SOCKET) {
//         std::cerr << "Socket creation failed.\n";
//         WSACleanup();
//         return 1;
//     }
//
//     // Configure server address structure
//     serverAddr.sin_family = AF_INET;
//     serverAddr.sin_port = htons(PORT);
//     inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);
//
//     // Connect to the server
//     if (connect(clientSocket, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
//         std::cerr << "Connection failed.\n";
//         closesocket(clientSocket);
//         WSACleanup();
//         return 1;
//     }
//
//     std::cout << "Connected to the server!\n";
//
//     // Read txt file
//     // std::ifstream file("text.txt");
//     // if (!file.is_open()) {
//     //     std::cerr << "File not found.\n";
//     //     closesocket(clientSocket);
//     //     WSACleanup();
//     //     return 1;
//     // }
//     //
//     // file.read(buffer, BUFFER_SIZE);
//
//     // Send and receive messages
//     while (true) {
//         std::cout << "Enter message: ";
//         std::cin.getline(buffer, BUFFER_SIZE);
//
//         // Send message to the server
//         send(clientSocket, buffer, static_cast<int>(strlen(buffer)), 0);
//
//         // Receive echo from the server
//         ZeroMemory(buffer, BUFFER_SIZE);
//         if (const int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0); bytesReceived > 0) {
//             std::cout << "Server echoed: " << buffer << "\n";
//         } else {
//             std::cout << "Connection closed by server.\n";
//             break;
//         }
//     }
//
//     // Cleanup
//     closesocket(clientSocket);
//     WSACleanup();
//
//     return 0;
// }
