#include "../include/SystemManager/OperatingSystemManager.h"
#include <random>

int main()
{
    size_t id = 0;
    const std::string cpu_s = OperatingSystemManager::GetClientCPUSerial();
    const std::string motherboard_s = OperatingSystemManager::GetClientMotherboardSerial();
    const std::string hdd_s = OperatingSystemManager::GetClientHDDSerial();

    id = std::hash<std::string>{}(cpu_s + motherboard_s + hdd_s);

    std::cout << "Your id:" << id << std::endl;
}
