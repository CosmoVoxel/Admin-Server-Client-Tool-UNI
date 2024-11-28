#pragma once
#include <Actions/ActionSystem.h>
#include <SystemManager/OperatingSystemManager.h>
#include <Actions/ActionStructures.h>

// ------------------------------ Actions Implementations ------------------------------ //
class RunCommand final : public BaseAction<CmdResult_S, CmdCommand_S>
{
public:
    RunCommand() : BaseAction("RunCommand", true) {}

protected:
    CmdResult_S perform() override
    {
        CmdResult_S cmd_command;
        cmd_command.result = OperatingSystemManager::ExecuteCommand(input_data.command);
        return cmd_command;
    }
};

// Реализация GetClientStatus с использованием шаблона
class GetClientStatus final : public BaseAction<PCStatus_S_OUT>
{
public:
    GetClientStatus() : BaseAction("GetClientStatus", false) {}

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
