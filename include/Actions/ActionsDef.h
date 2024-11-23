#pragma once
#include <future>
#include <iostream>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include <Actions/ActionTypes.h>
#include <Sys_utils/IOperatingSystem.h>

using json = nlohmann::json;


class Action {
public:
	Action() = default;
	virtual ~Action() = default;
public:
	virtual std::string getName() {return "Base Actions";};

	virtual void execute(std::function<void(const DataStruct &)> callback) = 0;

	virtual void initialize(const json &json){};

protected:
	void EnableDataIn() {
		requires_data_in = true;
	}

	void DisableDataIn() {
		requires_data_in = false;
	}

public:
	bool requires_data_in = true;
};

// --- Actions Factory ---
class ActionFactory {
public:
	using ActionCreator = std::function<std::shared_ptr<Action>()>;

	// Register an action by its class type
	template<typename T>
	void registerAction() {
		actionRegistry[std::type_index(typeid(T))] = []() { return std::make_shared<T>(); };
	}

	// Create an action by its class type
	template<typename T>
	std::shared_ptr<Action> createAction() const {
		auto it = actionRegistry.find(std::type_index(typeid(T)));
		if (it != actionRegistry.end()) {
			return it->second();
		}
		else {
			const std::string error = "Unknown action + " + std::string(typeid(T).name()) + " requested\n" +
			                          "You need to register the action first!\n";
			throw std::runtime_error(error);
		}
	}

	// Create an action by its type id
	std::shared_ptr<Action> createAction(const std::type_index &type) const {
		auto it = actionRegistry.find(type);
		if (it != actionRegistry.end()) {
			return it->second();
		}
		else {
			throw std::runtime_error("Unknown action type requested");
		}
	}

public:
	std::unordered_map<std::type_index, ActionCreator> actionRegistry;
};

// --- Actions Manager ---
class ActionManager {
public:
	explicit ActionManager(const ActionFactory &factory) : factory(factory) {
	}

	void executeActions(const nlohmann::json &request) const {
		// Parse actions from the request
		const auto &actions = request["actions"];
		std::vector<std::future<void> > asyncTasks;

		for (const auto &actionData: actions) {
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
						action->execute([](const auto &result) {
						});
					}));
				}
				else {
					action->execute([](const auto &result) {
					});
				}
			}
			catch (const std::exception &e) {
				// Handle invalid action (log error)
				std::cerr << "Error: " << e.what() << "\n";
			}
		}

		// Wait for all async tasks to complete
		for (auto &task: asyncTasks) {
			task.get();
		}
	}

	static void executeAction(const std::shared_ptr<Action> &action) {
		action->execute([](const auto &result) {
		});
	}

	static void StartMessageLoop() {
		// Start the message loop
		MSG msg;
		while (GetMessage(&msg, nullptr, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	static void StopMessageLoop() {
		PostQuitMessage(0);
	}

private:
	const ActionFactory &factory;

	// Create an action from a JSON request
	std::shared_ptr<Action> createActionFromRequest(const nlohmann::json &actionData) const {
		const std::string actionType = actionData["index"];

		// Interate over all reg. the Actions classes
		for (const auto &[typeIndex, actionCreator]: factory.actionRegistry) {
			auto action = actionCreator();
			if (action->getName() == actionType) {
				return action;
			}
		}

		throw std::runtime_error("Unknown action type: " + actionType);
	}
};