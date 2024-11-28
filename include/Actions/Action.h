#pragma once
#include <Actions/ActionSystem.h>
#include <SystemManager/OperatingSystemManager.h>
#include <Actions/ActionStructures.h>

// ------------------------------ Actions Implementations ------------------------------ //
class RunCommand final : public BaseAction<CmdResult_S, CmdCommand_S>
{
public:
    RunCommand() : BaseAction("RunCommand", true)
    {
    }

protected:
    CmdResult_S perform() override
    {
        CmdResult_S cmd_command;
        cmd_command.result = OperatingSystemManager::ExecuteCommand(input_data.command);
        return cmd_command;
    }
};

class GetClientStatus final : public BaseAction<PCStatus_S_OUT>
{
public:
    GetClientStatus() : BaseAction("GetClientStatus", false)
    {
    }

protected:
    PCStatus_S_OUT perform() override
    {
        PCStatus_S_OUT pc_status;
        pc_status.ip = OperatingSystemManager::GetClientIP();
        pc_status.mac = OperatingSystemManager::GetClientMAC();
        pc_status.os = OperatingSystemManager::GetClientOS();
        return pc_status;
    }
};

// ------------------------------ Actions Registration ------------------------------ //

//!TODO: Register all actions here
class ActionRegistry
{
public:
    using Actions = std::vector<std::shared_ptr<Action>>;

    Actions on_startup_actions = {
        std::make_shared<GetClientStatus>()
    };
    Actions client_actions = {
        std::make_shared<RunCommand>(),
        std::make_shared<GetClientStatus>()
    };
};

inline ActionRegistry action_registry;