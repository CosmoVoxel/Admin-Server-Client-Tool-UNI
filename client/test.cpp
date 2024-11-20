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


// --- Data Structure ---
class DataStruct {
public:
    virtual ~DataStruct() = default;
};

// Example struct for PrintAction input
struct PrintActionRequest : public DataStruct{
    std::string message;   // Required
    std::string color = "black"; // Optional with default

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(PrintActionRequest, message, color);

};

// Example struct for ShowImageAction input
struct ShowImageActionRequest : public DataStruct {
    std::string image_path; // Required

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ShowImageActionRequest, image_path);
};

// --- Base Action Class ---
class Action
{
public:
    Action() = default;

    virtual ~Action() = default;

    virtual std::string getName() {
        return "Base Action";
    }

    // Execute the action with a callback and data input
    virtual void execute(std::function<void(const DataStruct&)> callback) = 0;

    virtual void initialize(const json& json) {
        // Default implementation does nothing
    }

    void EnableDataIn() {
        requires_data_in = true;
    }
    void DisableDataIn() {
        requires_data_in = false;
    }

public:
    bool requires_data_in = true;

protected:
    std::shared_ptr<DataStruct> actionData;
};


// --- Derived Actions ---
class PrintAction final : public Action {
public:
    std::string getName() override {
        return "Print Action";
    }
    PrintAction() {
        EnableDataIn();
    }

    void execute(const std::function<void(const DataStruct&)> callback) override {
        // Convert to PrintActionRequest
        const PrintActionRequest* print_request_data = std::static_pointer_cast<PrintActionRequest>(actionData).get();
        std::cout << "Printing: " << print_request_data->message << "\n";
        callback({*print_request_data}); // No output
    }

    void initialize(const json &json) override {
        actionData = std::make_shared<PrintActionRequest>(json.get<PrintActionRequest>());
    }

};

// --- Show image on screen ---
class ShowImageAction final : public Action {
public:
    ShowImageAction() {
        EnableDataIn();
    }

    void execute(const std::function<void(const DataStruct &)> callback) override {
        const auto print_request_data = std::static_pointer_cast<ShowImageActionRequest>(actionData).get();
        // Some interface that will work with operating system.
        std::cout << "Showing image: " << print_request_data->image_path << "\n";
        callback({*print_request_data});
    };

    std::string getName() override {
        return "Show Image";
    };

    void initialize(const json &json) override {
        actionData = std::make_shared<ShowImageActionRequest>(json.get<ShowImageActionRequest>());
    };
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

    // Create an action by its type id
    std::shared_ptr<Action> createAction(const std::type_index& type) const {
        auto it = actionRegistry.find(type);
        if (it != actionRegistry.end()) {
            return it->second();
        } else {
            throw std::runtime_error("Unknown action type requested");
        }
    }

public:
    std::unordered_map<std::type_index, ActionCreator> actionRegistry;
};


// --- Action Manager ---
class ActionManager {
public:
    explicit ActionManager(const ActionFactory& factory) : factory(factory) {}

    void executeActions(const nlohmann::json& request) const {
        // Parse actions from the request
        const auto& actions = request["actions"];
        std::vector<std::future<void>> asyncTasks;

        for (const auto& actionData : actions) {
            try {
                // Create the action using the factory
                auto action = createActionFromRequest(actionData);
                const bool isAsync = actionData.value("async", false);

                // Extract data and initialize the data in action if required!
                if (action->requires_data_in) {
                    action->initialize(actionData["data"]);
                }

                // Execute the action
                if (isAsync) {
                    asyncTasks.push_back(std::async(std::launch::async, [action]() {
                        action->execute([](const auto& result) {});
                    }));
                } else {
                    action->execute([](const auto& result) {
                    });
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

    // Create an action from a JSON request
    std::shared_ptr<Action> createActionFromRequest(const nlohmann::json& actionData) const {
        const std::string actionType = actionData["index"];

        // Interate over all children of the Action class
        for (const auto& [typeIndex, actionCreator] : factory.actionRegistry) {
            auto action = actionCreator();
            if (action->getName() == actionType) {
                return action;
            }
        }

        throw std::runtime_error("Unknown action type: " + actionType);
    }
};

// --- Client Code ---
int main() {
    // Set up the factory and register actions
    ActionFactory factory;
    factory.registerAction<PrintAction>();
    factory.registerAction<ShowImageAction>();


    // ActionManager to handle execution
    ActionManager manager(factory);

    manager.executeActions(R"(
        {
            "actions": [
                {
                    "index": "Print Action",
                    "data": {
                        "message": "Hello, World!",
                        "color": "red"
                    }
                },
                {
                    "index": "Show Image",
                    "data": {
                        "image_path": "path/to/image.png"
                    }
                }
            ]
        }
    )"_json);

    return 0;
}
