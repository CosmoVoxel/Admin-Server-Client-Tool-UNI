#include "OperatingSystemManager.h"
#include <array>


std::string OperatingSystemManager::GetClientIP()
{
    const std::string command = "powershell -Command \"Get-WmiObject win32_networkadapterconfiguration | "
        "Where-Object {$_.IPAddress -ne $null} | "
        "Select-Object -ExpandProperty IPAddress | Select-Object -First 1\"";
    return ExecuteCommand(command);
}

std::string OperatingSystemManager::GetClientMAC()
{
    const std::string command = "powershell -Command \"Get-WmiObject win32_networkadapterconfiguration | "
        "Where-Object {$_.MACAddress -ne $null} | "
        "Select-Object -ExpandProperty MACAddress | Select-Object -First 1\"";
    return ExecuteCommand(command);
}

std::string OperatingSystemManager::GetClientOS()
{
    const std::string command = "powershell -Command \"(Get-WmiObject Win32_OperatingSystem).Caption\"";
    return ExecuteCommand(command);
}


std::string OperatingSystemManager::ExecuteCommand(const std::string& command)
{
    std::array<char, 128> buffer{};
    std::string result;
    FILE* pipe = _popen((command + " 2>&1").c_str(), "r"); // Redirect stderr to stdout
    if (!pipe) return "Error executing command";
    try
    {
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
        {
            result += buffer.data();
        }
    }
    catch (...)
    {
        _pclose(pipe);
        throw;
    }
    _pclose(pipe);
    return result;
}

std::string OperatingSystemManager::GetClientCPUSerial()
{
    const std::string s = ExecuteCommand("wmic cpu get processorid");
    return get_last_line(s);
}

std::string OperatingSystemManager::GetClientMotherboardSerial()
{
    const std::string s = ExecuteCommand("wmic baseboard get serialnumber");
    return get_last_line(s);
}

std::string OperatingSystemManager::GetClientHDDSerial()
{
    const std::string s = ExecuteCommand("wmic diskdrive get serialnumber");
    return get_last_line(s);
}
