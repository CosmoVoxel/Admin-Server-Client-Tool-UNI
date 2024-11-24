#pragma once
#include <iostream>
#include <string>
#include <typeindex>
#include <utility>

// Data types
#include <vector>
#include <queue>
#include <unordered_map>

// Multithreading
#include <future>

#include <Actions/ActionTypes.h>
#include <Sys_utils/IOperatingSystem.h>

using json = nlohmann::json;

class Action
{
  public:
    explicit Action(const std::string &name) : class_name(name) {};
    virtual ~Action() = default;

  public:
    virtual std::string getName()
    {
        return class_name;
    };

    std::string class_name;

    virtual void execute(std::function<void(const DataStruct &)> callback) = 0;

    virtual void initialize(const json &json) {};

  protected:
    void SetInputOn()
    {
        requires_data_in = true;
    }

    void SetInputOff()
    {
        requires_data_in = false;
    }

  public:
    bool requires_data_in = true;
};

// --- Actions Factory ---
class ActionFactory
{
  public:
    using ActionCreator = std::function<std::shared_ptr<Action>()>;

    // Register an action by its class type
    template <typename T> void registerAction()
    {
        // T must be derived from Action
        static_assert(std::is_base_of_v<Action, T>, "T must be derived from Action");
        actionRegistry[std::type_index(typeid(T))] = [] { return std::make_shared<T>();};
    }

    // Create an action by its class type
    template <typename T> std::shared_ptr<Action> createAction() const
    {
        auto it = actionRegistry.find(std::type_index(typeid(T)));
        if (it != actionRegistry.end())
        {
            return it->second();
        }
        else
        {
            const std::string error = "Unknown action + " + std::string(typeid(T).name()) + " requested\n" +
                                      "You need to register the action first!\n";
            throw std::runtime_error(error);
        }
    }

    // Create an action by its type id
    std::shared_ptr<Action> createAction(const std::type_index &type) const
    {
        auto it = actionRegistry.find(type);
        if (it != actionRegistry.end())
        {
            return it->second();
        }
        throw std::runtime_error("Unknown action type requested");
    }

  public:
    std::unordered_map<std::type_index, ActionCreator> actionRegistry;
};

class ActionManager
{
  public:
    /**
     * \brief Constructs an ActionManager with the given ActionFactory.
     * \param factory The ActionFactory to use for creating actions.
     */
    explicit ActionManager(ActionFactory &factory) : factory(&factory)
    {
        initialize();
    }

    /**
     * \brief Executes actions based on the provided JSON request.
     * \param request The JSON request containing action data.
     */
    void executeActions(const nlohmann::json &request) const
    {
        const auto &actions = request.at("actions");
        std::vector<std::future<void>> asyncTasks;
        for (const auto &actionData : actions)
        {
            try
            {
                // Create the action using the factory
                auto action = createActionFromRequest(actionData);
                const bool isAsync = actionData.value("async", false);

                // Extract data and initialize the data in action if required!
                if (action->requires_data_in)
                {
                    action->initialize(actionData.at("data"));
                }

                // Execute the action
                if (isAsync)
                {
                    asyncTasks.push_back(
                        std::async(std::launch::async, [action]() { action->execute([](const auto &result) {}); }));
                }
                else
                {
                    action->execute([](const auto &result) {});
                }
            }
            catch (const std::exception &e)
            {
                // Handle invalid action (log error)
                std::cerr << "Error: " << e.what() << "\n";
            }
        }

        // Wait for all async tasks to complete
        for (auto &task : asyncTasks)
        {
            task.get();
        }
    }

    /**
     * \brief Initializes/Register actions in "Actions.cpp".
     */
    void initialize() const;

    /**
     * \brief Executes a single action.
     * \param action The action to execute.
     */
    static void executeAction(const std::shared_ptr<Action> &action)
    {
        action->execute([](const auto &result) {});
    }

    /**
     * \brief Starts the message loop.
     */
    static void StartMessageLoop()
    {
        // Start the message loop
        MSG msg;
        while (GetMessage(&msg, nullptr, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    /**
     * \brief Stops the message loop.
     */
    static void StopMessageLoop()
    {
        PostQuitMessage(0);
    }

  private:
    std::shared_ptr<ActionFactory> factory;

    /**
     * \brief Creates an action from a JSON request.
     * \param actionData The JSON data for the action.
     * \return A shared pointer to the created action.
     * \throws std::runtime_error if the action type is unknown.
     */
    std::shared_ptr<Action> createActionFromRequest(const nlohmann::json &actionData) const
    {
        const std::string actionType = actionData.at("index").get<std::string>();

        // Interate over all reg. the Actions classes
        for (const auto &actionCreator : factory->actionRegistry | std::views::values)
        {
            auto action = actionCreator();
            if (action->getName() == actionType)
            {
                return action;
            }
        }

        throw std::runtime_error("Unknown action type: " + actionType);
    }

    // We probably need some king of queue to store the actions.
    std::queue<std::shared_ptr<Action>> actionQueue;
};