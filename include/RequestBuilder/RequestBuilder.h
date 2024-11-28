#pragma once
#include <Actions/ActionSystem.h>

class Request
{
public:
    explicit Request() = default;

    explicit Request(const Action& action, const json& data)
    {
        InitializeRequest(action.class_name, data);
    }

    explicit Request(const std::string& action_name, const json& data)
    {
        InitializeRequest(action_name, data);
    }

    explicit Request(const std::string& action_name)
    {
        InitializeRequest(action_name, json{});
    }

    void InitializeRequest(const std::string& action_name, const json& data)
    {
        json result;

        result["transaction_id"] = transaction_id;
        this->action_name = action_name;
        result["index"] = action_name;
        if (!data.empty())
        {
            result["data"] = data;
        }

        body = result.dump();
    }

    static nlohmann::json GetJsonFromRequest(const Request& request);

    void Encrypt();
    void Decrypt();

public:
    std::string action_name;
    int transaction_id = static_cast<int>(std::chrono::system_clock::now().time_since_epoch().count());
    // Generate a unique transaction id. :) NOT REALLY
    std::string body;
};

inline nlohmann::json Request::GetJsonFromRequest(const Request& request)
{
    json result = json::parse(request.body);
    result["transaction_id"] = request.transaction_id;
    return result;
}
