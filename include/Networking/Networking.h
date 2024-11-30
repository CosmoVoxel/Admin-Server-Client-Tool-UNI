#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

enum SendDataResult
{
    DataSent,
    DataNotSent,
    UnknownSentError
};

inline SendDataResult SendData(
    const SOCKET socket,
    const std::variant<std::string,json>&data,
    const int max_retries = 10,
    const int wait_time = 3)
{
    const auto& actual_data = data.index() == 0 ? std::get<std::string>(data) : std::get<json>(data).dump();
    try
    {
        for (int attempt = 0; attempt < max_retries; ++attempt)
        {
            const int bytes_sent = send(socket, actual_data.c_str(), actual_data.size(), 0);
            if (bytes_sent != SOCKET_ERROR)
            {
                return DataSent;
            }
            std::this_thread::sleep_for(std::chrono::seconds(wait_time));
        }
    }
    catch (const std::exception&)
    {
        return UnknownSentError;
    }
    return DataNotSent;
}


enum RecvDataResult
{
    DataReceived,
    DataNotReceived,
    UnknownReceivedError
};

inline RecvDataResult RecvData(
    const SOCKET socket,
    std::string& data,
    const int buffer_size = 1024,
    const int max_retries = 10,
    const int wait_time = 3)
{
    try
    {
        char buffer[buffer_size];
        for (int attempt = 0; attempt < max_retries; ++attempt)
        {
            const int bytes_received = recv(socket, buffer, buffer_size, 0);
            if (bytes_received > 0)
            {
                data.assign(buffer, bytes_received);
                return DataReceived;
            }
            std::this_thread::sleep_for(std::chrono::seconds(wait_time));
        }
    }
    catch (const std::exception&)
    {
        return UnknownReceivedError;
    }
    return DataNotReceived;
}