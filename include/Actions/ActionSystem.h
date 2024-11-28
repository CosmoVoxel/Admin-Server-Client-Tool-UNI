#pragma once
#include <iostream>
#include <typeindex>
#include <utility>

// Data types
#include <vector>
#include <unordered_map>
#include <json/json.hpp>

// Multithreading
#include <future>

#include "ActionStructures.h"


using json = nlohmann::json;

class Action
{
public:
    explicit Action(const std::string& name, bool requires_data = true)
        : class_name(name), requires_data_in(requires_data)
    {
    }

    virtual ~Action() = default;

    const std::string& getName() const { return class_name; }

    virtual void execute(std::function<void(const json&)> callback) = 0;

    virtual void initialize(const json& json) = 0;
    virtual std::any deserialize(const json& data) = 0;
    virtual json serialize() = 0;

public:
    bool requires_data_in;
    std::string class_name;
};

template <typename OutDataType, typename InDataType = void>
class BaseAction : public Action
{
public:
    using out_data_type = OutDataType;
    using in_data_type = InDataType;

    explicit BaseAction(const std::string& name, bool requires_data = true)
        : Action(name, requires_data)
    {
    }

    void execute(std::function<void(const json&)> callback) override
    {
        try
        {
            OutDataType result = perform();
            callback(result);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error in action '" << class_name << "': " << e.what() << "\n";
            callback({});
        }
    }

    void initialize(const json& json) override
    {
        if constexpr (!std::is_void_v<InDataType>)
        {
            input_data = json.get<InDataType>();
        }
    }

    std::any deserialize(const json& data) override
    {
        return data.get<OutDataType>();
    }

    // Type in field for InDataType
    json serialize() override
    {
        if constexpr (std::is_void_v<InDataType>)
        {
            // Return a monostate when InDataType is void (no actual data to serialize)
            return {};
        }
        else
        {
            // Handle the case where InDataType is not void
            InDataType in_data;
            json j = in_data;

            std::string string;
            for (auto& [key, value] : j.items())
            {
                std::cout << "Enter " << key << ": ";
                std::getline(std::cin, string);
                j[key] = string;
            }

            return j;
        }
    }

protected:
    virtual OutDataType perform() = 0;

    using InputDataType = std::conditional_t<std::is_void_v<InDataType>, std::monostate, InDataType>;
    InputDataType input_data{};
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
