#pragma once
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>


#if _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif


#include <Actions/ActionStructures.h>
#include <Actions/ActionSystem.h>
#include <Networking/Networking.h>
#include <RequestBuilder/RequestBuilder.h>
#include <Actions/Action.h>

#pragma comment(lib, "Ws2_32.lib")

using json = nlohmann::json;

struct ClientThreadData {
    size_t id{};
    bool is_admin = false;
    SOCKET client_socket{};

    PCStatus_S_OUT status;
    bool is_client_connected = false;

    size_t last_status_update_time = 0; // UNIX timestamp

    std::queue<int> action_queue;

    void update_status_time() {
        last_status_update_time = static_cast<size_t>(std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now()));
    }
};

class Server {
public:
    Server();

    ~Server();

#if _WIN32
    WSADATA wsaData{};
#endif

    void StartServer();

    void EndServer();

    void HandleClient();

    void AdminThread(Server *server);

    //---------============ HELPERS============---------//
    static std::optional<json> ReceiveAndParseResponse(int client_socket, std::string &buffer);

    static void ProcessClientAction(size_t id, ClientThreadData *thread_data, const Request &request,
                                    const json &action_data);

    void HandleClientAction(size_t client_id, const Request &request);

    void BroadcastAction(const Request &request, const json &action_data);

public:
    ActionFactory actionFactory;
    SOCKET server_socket{};
    sockaddr_in serverAddr{};
    int PORT = 54000;
    bool isRunning = false;

    std::string admin_secret = "admin:admin";

protected:
    std::thread adminThread;
    std::mutex admin_thread_mutex;

    std::map<size_t, std::pair<std::unique_ptr<ClientThreadData>, std::thread> > client_threads;

    using Actions = std::vector<std::shared_ptr<Action> >;

    static void PrintAllActionsWithIndex(const Actions &actions);;
};
