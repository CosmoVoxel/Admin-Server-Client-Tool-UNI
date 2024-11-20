//
// Created by ERIC CARTMAN on 20/11/2024.
//
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

#include <typeindex>
#include <typeinfo>

#include <json.hpp>

#include <future>
#include <thread>
#include <variant>
#include <vector>




// Use for convenience
using json = nlohmann::json;

// --- Data Representation ---
using Data = std::variant<std::string, int, double>;



struct DataStruct {

};

class RequestBuilder {
public:
    // Generic function to parse a request into a struct
    template <typename RequestStruct>
    static RequestStruct parseRequest(const nlohmann::json& json) {
        // Use nlohmann::json's deserialization to convert the JSON into the struct
        RequestStruct request;
        try {
            // Try to deserialize directly into the struct
            request = json.get<RequestStruct>();
        } catch (const std::exception& e) {
            throw std::runtime_error("Error parsing request: " + std::string(e.what()));
        }
        return request;
    }

    // Function to build a request from a struct and validate
    template <typename RequestStruct>
    static nlohmann::json buildRequest(const RequestStruct& request) {
        nlohmann::json jsonRequest = request;
        return jsonRequest;
    }
};


// --- Base Action Class ---
class Action {
public:
    virtual ~Action() = default;

    static std::string getName() {
        return "Base Action";
    }

    // Execute the action
    virtual void execute(const DataStruct& input, std::function<void(const Data&)> callback) = 0;
};

// Example struct for PrintAction input
struct PrintActionRequest : public DataStruct{
    std::string message;   // Required
    int font_size = 12;    // Optional with default
    std::string color = "black"; // Optional with default
    bool bold = false;     // Optional with default
};

// --- Derived Actions ---
class PrintAction : public Action {
public:
    static std::string getName() {
        return "Print Action";
    }

    void execute(const DataStruct& input, std::function<void(const Data&)> callback) override {
        // Convert to PrintActionRequest
        const auto& printRequest = reinterpret_cast<const PrintActionRequest&>(input);
        std::cout << "PrintAction: " << printRequest.message << "\n";
        callback({}); // No output
    }
};

// --- Action Factory ---
class ActionFactory {
public:
    using ActionCreator = std::function<std::shared_ptr<Action>()>;

    // Register an action by its class type
    template <typename T>
    void registerAction() {
        actionRegistry[std::type_index(typeid(T))] = []() { return std::make_shared<T>(); };
    }

    // Create an action by its class type
    template <typename T>
    std::shared_ptr<Action> createAction() const {
        auto it = actionRegistry.find(std::type_index(typeid(T)));
        if (it != actionRegistry.end()) {
            return it->second();
        } else {
            throw std::runtime_error("Unknown action type requested");
        }
    }

private:
    std::unordered_map<std::type_index, ActionCreator> actionRegistry;
};


// --- Action Manager ---
class ActionManager {
public:
    explicit ActionManager(const ActionFactory& factory) : factory(factory) {}

    void executeActions(const nlohmann::json& request) {
        // Parse actions from the request
        const auto& actions = request["actions"];
        std::vector<std::future<void>> asyncTasks;

        for (const auto& actionData : actions) {
            try {
                // Create the action using the factory
                auto action = createActionFromRequest(actionData);
                bool isAsync = actionData.value("async", false);

                // Extract input data for execution
                std::variant<std::string, int, double> input = parseInput(actionData);

                // Execute the action
                if (isAsync) {
                    asyncTasks.push_back(std::async(std::launch::async, [action, input]() {
                        action->execute(input, [](const auto& result) {});
                    }));
                } else {
                    action->execute(input, [](const auto& result) {});
                }
            } catch (const std::exception& e) {
                // Handle invalid action (log error)
                std::cerr << "Error: " << e.what() << "\n";
            }
        }

        // Wait for all async tasks to complete
        for (auto& task : asyncTasks) {
            task.get();
        }
    }



private:
    const ActionFactory& factory;

    // // Parse input data for the action
    // static std::variant<std::string, int, double> parseInput(const nlohmann::json& actionData) {
    //     if (actionData.contains("input")) {
    //         const auto& input = actionData["input"];
    //         if (input.is_string()) return input.get<std::string>();
    //         if (input.is_number_integer()) return input.get<int>();
    //         if (input.is_number_float()) return input.get<double>();
    //     }
    //     return {}; // Default to empty
    // }

    // Create an action from a JSON request
    std::shared_ptr<Action> createActionFromRequest(const nlohmann::json& actionData) {
        const std::string actionType = actionData["type"];
        if (actionType == "Print") return factory.createAction<PrintAction>();
        if (actionType == "Multiply") return factory.createAction<MultiplyAction>();

        throw std::runtime_error("Unknown action type: " + actionType);
    }
};
// --- Client Code ---
int main() {
    // Set up the factory and register actions
    ActionFactory factory;
    factory.registerAction<PrintAction>();
    factory.registerAction<MultiplyAction>();

    // ActionManager to handle execution
    ActionManager manager(factory);

    // Sample request JSON
    nlohmann::json request = R"({
        "actions": [
            { "type": "Print", "input": "Hello, world!", "async": false },
            { "type": "Multiply", "input": 5, "async": true }
        ]
    })"_json;

    // Execute the actions
    manager.executeActions(request);

    return 0;
}
