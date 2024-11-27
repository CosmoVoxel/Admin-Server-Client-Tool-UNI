#include <Actions/ActionSystem.h>
#include <SystemManager/IOperatingSystem.h>
// ------------------------------ Actions Implementations ------------------------------ //
class RunCommand final : public Action
{
public:
    RunCommand(): Action("RunCommand")
    {
        SetInputOn();
    }


    void execute(const std::function<void(const json&)> callback) override
    {
        try
        {
            CmdResult_S cmd_command = OperatingSystemManager::RunCMDCommand(command.command);
            callback(cmd_command);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error: " << e.what() << "\n";
            CmdResult_S cmd_command;
            cmd_command.result = "Error: " + std::string(e.what());
            callback({cmd_command});
        }
    };

    void initialize(const json& json) override
    {
        command = json.get<CmdCommand_S>();
    };

    CmdCommand_S command{};
};


//!TODO: Add more actions here. For example, RunCommand, CaptureScreen, etc.
void ActionManager::RegisterActions() const
{
    factory->registerAction<RunCommand>();
}
