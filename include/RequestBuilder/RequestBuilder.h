#pragma once
#include <Actions/ActionsSystem.h>

class RequestBuilder
{
public:
    // Depending on the data structure, we can create a request
    template <typename Data>
    static void CreateRequest(Action& action);
};


template <typename Data>
void RequestBuilder::CreateRequest(Action& action)
{
    // Create a request
    json request;
    request["data"] = Data();
    request["index"] = action.getName();

    std::cout << "Request created: " << request.dump() << std::endl;
}
