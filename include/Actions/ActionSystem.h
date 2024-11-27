#pragma once
#include <iostream>
#include <string>
#include <typeindex>
#include <utility>

// Data types
#include <vector>
#include <unordered_map>
#include <json/json.hpp>
#include <Actions/ActionStructures.h>

// Multithreading
#include <future>


using json = nlohmann::json;


class Action
{
public:
    explicit Action(const std::string& name) : class_name(name)
    {
    };
    virtual ~Action() = default;

public:
    virtual std::string getName()
    {
        return class_name;
    };

    std::string class_name;

    virtual void execute(std::function<void(const json&)> callback) = 0;

    virtual void initialize(const json& json)
    {
    };

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
    template <typename T>
    void registerAction()
    {
        // T must be derived from Action
        static_assert(std::is_base_of_v<Action, T>, "T must be derived from Action");
        actionRegistry[std::type_index(typeid(T))] = [] { return std::make_shared<T>(); };
    }

    // Create an action by its class type
    template <typename T>
    std::shared_ptr<Action> createAction() const
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
    std::shared_ptr<Action> createAction(const std::type_index& type) const
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
    explicit ActionManager(ActionFactory& factory) : factory(&factory)
    {
        RegisterActions();
    }

    void RegisterActions() const;

    void executeActions(const nlohmann::json& request);

    json executeAction(const nlohmann::json& action_json);

    std::shared_ptr<Action> createActionFromRequest(const nlohmann::json& actionData) const;

private:
    std::shared_ptr<ActionFactory> factory;



    std::vector<std::shared_ptr<Action>> active_actions;
};

