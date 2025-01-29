#include "OperatingSystemManager.h"

#include <format>
#if defined(_WIN32) || defined(_WIN64)

#else
#define _popen popen
#define _pclose pclose
#endif


std::string OperatingSystemManager::ExecuteCommand(const std::string &command) {
    std::array<char, 128> buffer{};
    std::string result;
    FILE *pipe = _popen((command + " 2>&1").c_str(), "r"); // Redirect stderr to stdout
    if (!pipe) return "Error executing command";
    try {
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            result += buffer.data();
        }
    } catch (...) {
        _pclose(pipe);
        throw;
    }
    _pclose(pipe);
    return result;
}

std::string OperatingSystemManager::GetClientIP() {
#if _WIN32
    const std::string command = "powershell -Command \"Get-WmiObject win32_networkadapterconfiguration | "
            "Where-Object {$_.IPAddress -ne $null} | "
            "Select-Object -ExpandProperty IPAddress | Select-Object -First 1\"";
#else
    const std::string command = "ifconfig | grep 'inet ' | awk '{print $2}'";
#endif
    return ExecuteCommand(command);
}

std::string OperatingSystemManager::GetClientMAC() {
#if _WIN32
    const std::string command = "powershell -Command \"Get-WmiObject win32_networkadapterconfiguration | "
            "Where-Object {$_.MACAddress -ne $null} | "
            "Select-Object -ExpandProperty MACAddress | Select-Object -First 1\"";
#else
    const std::string command = "ip -o link | awk '{print $2 $17}'";
#endif

    return ExecuteCommand(command);
}

std::string OperatingSystemManager::GetClientOS() {
#if _WIN32
    const std::string command = "powershell -Command \"(Get-WmiObject Win32_OperatingSystem).Caption\"";
#else
    const std::string command = "grep ^PRETTY_NAME /etc/os-release | cut -d= -f2 | tr -d ''";
#endif

    return ExecuteCommand(command);
}

std::string OperatingSystemManager::GetClientCPUSerial() {
#if _WIN32
    const std::string s = ExecuteCommand("wmic cpu get processorid");
#else

    unsigned int eax, ebx, ecx, edx;

    // CPUID with EAX=1 to get Processor ID
    __asm__ __volatile__(
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(1)
    );

    // Combine EDX and EAX to form a Processor ID
    unsigned long long processor_id = ((unsigned long long) edx << 32) | eax;

    std::string s = std::format("{:x}", processor_id);
    std::ranges::transform(s.begin(), s.end(), s.begin(), ::toupper);

#endif

    return get_last_line(s);
}

std::string OperatingSystemManager::GetClientMotherboardSerial() {
    const std::string s = ExecuteCommand("wmic baseboard get serialnumber");
    return get_last_line(s);
}

std::string OperatingSystemManager::GetClientHDDSerial() {
    const std::string s = ExecuteCommand("wmic diskdrive get serialnumber");
    return get_last_line(s);
}
