//
// Created by ERIC CARTMAN on 17/11/2024.
//

#ifndef CLASS_H
#define CLASS_H

#endif //CLASS_H

#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

class Server {
public:
	Server();

	~Server();

	WSADATA wsaData{};
	SOCKET serverSocket{}, clientSocket{};
	sockaddr_in serverAddr{}, clientAddr{};

	int PORT = 54000;
	bool is_running = false;

	void StartServer();

	void EndServer();

	static void HandleClient(SOCKET clientSocket);

	void ControlClient();

	bool SendMessageToUser(int from, int to, const std::string &message);

	std::vector<std::thread> clients_threads;
	std::thread control_thread;

	std::string receive();
};


inline Server::Server() {
	std::cout << "Server created" << std::endl;
}

inline Server::~Server() {
	std::cout << "Server deleted" << std::endl;
}

inline void Server::StartServer() {
	/* Windows socket only*/
	// Initialize Winsock
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "WSAStartup failed.\n";
		EndServer();
	}

	// Create server socket
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == INVALID_SOCKET) {
		std::cerr << "Socket creation failed.\n";
		EndServer();
	}

	// Configure server address structure
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);
	serverAddr.sin_addr.s_addr = INADDR_ANY; // 127.0.0.1

	// Bind the socket
	if (bind(serverSocket, (sockaddr *) &serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "Bind failed.\n";
		EndServer();
	}

	// Listen for incoming connections
	if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
		std::cerr << "Listen failed.\n";
		EndServer();
	}

	std::cout << "Server is listening on port " << PORT << "...\n";

	is_running = true;

	// Control thread
	control_thread = std::thread(&Server::ControlClient, this);

	while (is_running) {
		int clientSize = sizeof(clientAddr);
		clientSocket = accept(serverSocket, (sockaddr *) &clientAddr, &clientSize);
		if (clientSocket == INVALID_SOCKET) {
			std::cerr << "Client connection failed.\n";
		}

		std::cout << "Client connected!" << "With IP: " << inet_ntoa(clientAddr.sin_addr) << std::endl;
		std::cout << "Moving to client handling to thread" << std::endl;

		clients_threads.emplace_back(HandleClient, clientSocket);

	}
}

inline void Server::EndServer() {
	// Close all client threads
	for (auto &clients_thread: clients_threads) {
		if (clients_thread.joinable()) {
			clients_thread.join();
		}
	}
	// Cleanup
	is_running = false;
	closesocket(clientSocket);
	closesocket(serverSocket);
	WSACleanup();
}

inline void Server::HandleClient(const SOCKET clientSocket) {
	char buffer[4096];
	while (true) {
		ZeroMemory(buffer, sizeof(buffer));
		int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
		if (bytesReceived == SOCKET_ERROR || bytesReceived == 0) {
			std::cout << "Connection closed.\n";
			break;
		}



		std::cout << "Received: " << buffer << "\n";

		// Echo the message back
		send(clientSocket, buffer, bytesReceived, 0);
	}
}

enum Action {
	SendMessageToUser,
	SendMessageToAllUsers
};

// Creating > operator for Action enum
inline std::ostream &operator<<(std::ostream &os, const Action &action) {
	switch (action) {
		case SendMessageToUser:
			os << "Send message to user";
			break;
		case SendMessageToAllUsers:
			os << "Send message to all users";
			break;
	}
	return os;
}


inline void Server::ControlClient() {
	/* Creating corresponding client-admin interface*/

	std::cout << "Select the action you want to perform: " << std::endl;
	std::cout << "1. Send message to user" << std::endl;
	std::cout << "2. Send message to all users" << std::endl;

	int choice;
	std::cin >> choice;

	Action action = static_cast<Action>(choice + 1);

	switch (action) {
		case Action::SendMessageToUser:
			// Print all users
			for (auto clients_thread: clients_threads) {
				std::cout << "Client id: " << clients_thread.get_id() << std::endl;
			}

			std::cout << "Enter the user id: ";

			std::cout << "Enter the message: ";


			// SendMessageToUser(0, userId, message);
			break;
		case Action::SendMessageToAllUsers:
			std::cout << "Enter the message: ";
			// SendMessageToAllUsers(0, messageAll);
			break;
	}


}


inline bool Server::SendMessageToUser(int from, int to, const std::string &message) {
	return true;
}

inline std::string Server::receive() {
	return "hello";
}
