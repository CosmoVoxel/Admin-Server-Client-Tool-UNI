//
// Created by ERIC CARTMAN on 18/11/2024.
//

#ifndef ACTIONS_H
#define ACTIONS_H

#include <data.h>
#include <iostream>
#include <vector>

enum ActionEnum {
	defaultAction = 0,
	SendHelloToUser = 1,
	SetMousePosition = 2,
	ShowSpookyImage = 3
};

class Actions {
public:
	Actions()
		: action(defaultAction), name("Unnamed Action"), description("No description provided"){};

	virtual ~Actions();

public:
	virtual void Action() = 0;

	void AddRequiredData(const Data& data) {
		required_data.push_back(data);
	};
	void AddRequiredData(const std::vector<Data&>& data) {
		required_data.insert(required_data.end(), data.begin(), data.end());
	};
	// Set the 1 output data
	void AddOutputData(const Data& data) {
		required_data.push_back(data);
	}
	// Append vector of Data to the output data
	void AddOutputData(const std::vector<Data>& data) {
		required_data.insert(required_data.end(), data.begin(), data.end());
	}

	std::vector<Data> GetRequiredData() {
		return required_data;
	};

	std::vector<Data> GetOutputData() {
		return output_data;
	}


public:
	ActionEnum action;
	std::string name;
	std::string description;

private:
	std::vector<Data> required_data;
	std::vector<Data> current_data{};
	std::vector<Data> output_data;

};


class SetMousePosition final : public Actions {
public:
	SetMousePosition(const Position& position, const Text& text)
	{
		action = ActionEnum::SetMousePosition;
		name = "SetMousePosition";
		description = "Sets the mouse position to the specified coordinates";

		// Configure the required data
		this->position = position;
		this->text = text;
	}


	void Action() override
	{
		// Do smf
		SetLocalMousePosition();
	}
private:
	void SetLocalMousePosition() const
	{
		// Set the mouse position
		std::cout << "Mouse position set to: "<<"X: " <<position.x << std::endl;
	}

	Position position;
	Text text;

};

class ActionExecManager {
public:
	static bool ExecuteAction(const json& data) {
		// Convert the string to the required data

		if (data.contains("globalInfo")) {
			std::cout << "Contains GlobalInfo" << std::endl;
			// Interate over the global data
			for (const auto& [key , value] : data["GlobalInfo"].items()) {
				std::cout << key << " : " << value << std::endl;
			}
		}

		if (data.contains("actions")) {
			std::cout << "Contains Actions" << std::endl;

			// Iterate over the actions
			for (const auto& action : data["actions"]) {
				std::cout << "Action ID: " << action["id"] << std::endl;


				// Iterate over details of the action
				for (const auto& detail : action["details"]) {
					std::cout << "Class: " << detail["data_class"] << std::endl;
					std::cout << "Data: " << detail["data"] << std::endl;

				}
			}

			data.get<std::vector<std::string>>("details");

		// Execute the action
		/*{
			"Actions":[
			{
			"Action_id" : 1,

			"Data":[
			"DataType": "Position",
			{
				"X": 100,
				"Y": 200,
			},
			{
			"DataType": "Text",
			{
				"Text": "Hello World",
			}
			}]
			"LocalAdditionalData": {
				"Silent": true,
				"InNewThread": false,
				"Wait": 10, // In Sec
				"Repeat": 1,
			}
			},
			{
			"Action_id" : 2,
			"Data":[
			...
			}],
			GlobalAdditionalData: {
				"Silent": true,
				"InNewThread": false,
				"Wait": 10, // In Sec
				"Repeat": 1,
			}
		}*/
	}
};








#endif //ACTIONS_H
