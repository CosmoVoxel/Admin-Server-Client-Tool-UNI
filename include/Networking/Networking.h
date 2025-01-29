#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SOCKET int
#define SOCKET_ERROR (-1)
#define INVALID_SOCKET (-1)
#define closesocket(socket) close(socket)
#define WSACleanup()

#endif


enum class DataStatus {
    DataSent = 0,
    DataNotSent = 1,
    DataReceived = 2,
    DataNotReceived = 3,
    UnknownSentError = 4,
    UnknownReceivedError = 5
};

template<typename FlagType = std::atomic<bool> >
DataStatus SendData(
    const SOCKET &socket,
    const std::variant<std::string, json> &data,
    std::optional<std::shared_ptr<FlagType> > success_flag = std::nullopt,
    const int max_retries = 5,
    const int wait_time = 1) {
    const auto &actual_data = data.index() == 0 ? std::get<std::string>(data) : std::get<json>(data).dump();
    try {
        for (int attempt = 0; attempt < max_retries; ++attempt) {
            // Check if socket is closed
            if (socket == INVALID_SOCKET) {
                if (success_flag) success_flag.value()->store(false, std::memory_order_relaxed);
                return DataStatus::DataNotSent;
            }
            const int bytes_sent = send(socket, actual_data.c_str(), static_cast<int>(actual_data.size()), 0);
            if (bytes_sent != SOCKET_ERROR) {
                if (success_flag) success_flag.value()->store(true, std::memory_order_relaxed);
                return DataStatus::DataSent;
            }

            // Print error and retry
            std::cout << "Error... Failed to send to socket: " << socket << ". Attempt: " << attempt + 1 << "\n";
            if (success_flag) success_flag.value()->store(false, std::memory_order_relaxed);
            std::this_thread::sleep_for(std::chrono::seconds(wait_time));
        }
    } catch (const std::exception &) {
        if (success_flag) success_flag.value()->store(false, std::memory_order_relaxed);
        return DataStatus::UnknownSentError;
    }

    if (success_flag) success_flag.value()->store(false, std::memory_order_relaxed);
    return DataStatus::DataNotSent;
}

template<typename FlagType = std::atomic<bool> >
DataStatus RecvData(
    const SOCKET &socket,
    std::string &data,
    std::optional<std::shared_ptr<FlagType> > success_flag = std::nullopt,
    const int buffer_size = 1024,
    const int max_retries = 5,
    const int wait_time = 1) {
    try {
        char buffer[buffer_size];
        for (int attempt = 0; attempt < max_retries; ++attempt) {
            // Check if socket is closed
            if (socket == INVALID_SOCKET) {
                if (success_flag) success_flag.value()->store(false, std::memory_order_relaxed);
                return DataStatus::DataNotSent;
            }

            const int bytes_received = recv(socket, buffer, buffer_size, 0);
            if (bytes_received > 0) {
                data.assign(buffer, bytes_received);
                if (success_flag) success_flag.value()->store(true, std::memory_order_relaxed);
                return DataStatus::DataReceived;
            }

            // Print error and retry
            std::cout << "Error... Failed to receive from socket: " << socket << ". Attempt: " << attempt + 1 << "\n";
            if (success_flag) success_flag.value()->store(false, std::memory_order_relaxed);
            std::this_thread::sleep_for(std::chrono::seconds(wait_time));
        }
    } catch (const std::exception &) {
        if (success_flag) success_flag.value()->store(false, std::memory_order_relaxed);
        return DataStatus::UnknownReceivedError;
    }

    if (success_flag) success_flag.value()->store(false, std::memory_order_relaxed);
    return DataStatus::DataNotReceived;
}
