#include <functional>
#include <iostream>
#include <string>
#include "../include/json/json.hpp"

// Mock definitions for demonstration
struct PCStatus_S_OUT {
    std::string ip;
    std::string mac;
    std::string os;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(PCStatus_S_OUT, ip, mac, os);
};

struct MousePosition_S_OUT {
    int x;
    int y;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(MousePosition_S_OUT, x, y);
};

class OperatingSystemManager {
public:
    static std::string GetClientIP() { return "192.168.1.1"; }
    static std::string GetClientMAC() { return "00:1A:2B:3C:4D:5E"; }
    static std::string GetClientOS() { return "Linux"; }
};

class Action {
public:
    virtual ~Action() = default;
    virtual void execute(std::function<void(const std::string&)> callback) = 0;

protected:
    void SetInputOff() {}
};

class GetClientStatus final : public Action {
public:
    GetClientStatus() : Action() { SetInputOff(); }

    // Expose type alias
    using struct_type = PCStatus_S_OUT;

    void execute(std::function<void(const std::string&)> callback) override {
        try {
            pc_status.ip = OperatingSystemManager::GetClientIP();
            pc_status.mac = OperatingSystemManager::GetClientMAC();
            pc_status.os = OperatingSystemManager::GetClientOS();
            callback(pc_status.ip); // Simplified callback for demonstration
        } catch (const std::exception&) {
            callback("Error");
        }
    }

private:
    PCStatus_S_OUT pc_status;
};

class GetMousePosition final : public Action
{
public:
    void execute(std::function<void(const std::string&)> callback) override
    {
        mouse_position.x = 10;
        mouse_position.y = 20;
        callback(std::to_string(mouse_position.x));
    };

    // Expose type alias
    using struct_type = MousePosition_S_OUT;

private:
    MousePosition_S_OUT mouse_position{};
};

// Generic function to use with struct_type
template <typename T>
void do_smf() {
    typename T::struct_type instance;
    std::cout << "Performing operations on struct_type instance..." << std::endl;
    nlohmann::json j = instance;
    std::cout << j.dump(4) << std::endl;
    std::cout << "Operations completed." << std::endl;

}

int main() {
    MousePosition_S_OUT mouse_position;
    nlohmann::json j = mouse_position;


    for (const auto& [key, value] : j.items()) {
        std::cout << key << " : " << value << std::endl;
    }

    return 0;
}
