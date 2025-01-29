#include "ActionSystem.h"
#include <Actions/Action.h>

void ActionManager::executeActions(const nlohmann::json &request) {
    for (const auto &action_json: request.at("actions")) {
        try {
            // Create the action using the factory
            auto action = createActionFromRequest(action_json);

            // Extract data and initialize the data in action if required!
            if (action->requires_data_in) {
                action->initialize(action_json.at("data"));
            }

            // Save the action to keep it alive
            active_actions.push_back(action);

            // Execute the action synchronously
            action->execute([this, action](const auto &result) {
                //!TODO: Send result back to the server

                // Remove the action from the active actions
                std::erase(active_actions, action);
            });
        } catch (const std::exception &e) {
            // Handle invalid action (log error)
            std::cerr << "Error: " << e.what() << "\n";
        }
    }
}

json ActionManager::executeAction(const nlohmann::json &action_json) {
    try {
        // Create the action using the factory
        auto action = createActionFromRequest(action_json);

        // Extract data and initialize the data in action if required!
        if (action->requires_data_in) {
            action->initialize(action_json.at("data"));
        }

        // Save the action to keep it alive
        active_actions.push_back(action);
        // Execute the action synchronously
        json result{};
        action->execute([this, action,&result](const json &res) {
            // Remove the action from the active actions
            std::erase(active_actions, action);

            result = res;
        });
        return result;
    } catch (const std::exception &e) {
        // Handle invalid action (log error)
        std::cerr << "Error: " << e.what() << "\n";
    }
    return {};
}

std::shared_ptr<Action> ActionManager::createActionFromRequest(const nlohmann::json &actionData) const {
    const std::string actionType = actionData.at("index").get<std::string>();

    // Interate over all reg. the Actions classes
    for (const auto &actionCreator: factory->actionRegistry | std::views::values) {
        auto action = actionCreator();
        if (action->getName() == actionType) {
            return action;
        }
    }

    throw std::runtime_error("Unknown action type: " + actionType);
}

//!TODO: Add more actions here. For example, RunCommand, CaptureScreen, etc.
void ActionManager::RegisterActions() const {
    factory->registerAction<RunCommand>();
    factory->registerAction<GetClientStatus>();
}

