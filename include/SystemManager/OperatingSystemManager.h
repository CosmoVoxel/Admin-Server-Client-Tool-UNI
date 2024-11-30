#pragma once
#include <fstream>
#include <iostream>


class OperatingSystemManager final
{
public:
    static std::string GetClientIP();
    static std::string GetClientMAC();
    static std::string GetClientOS();

    static std::string ExecuteCommand(const std::string& command);

    static std::string GetClientCPUSerial();
    static std::string GetClientMotherboardSerial();
    static std::string GetClientHDDSerial();

private:
    static std::string get_last_line(const std::string& str) {
        const size_t pos = str.find_last_not_of("\n\r");
        if (pos == std::string::npos) {
            return str; // No non-newline character found, return the original string
        }
        const size_t start = str.find_last_of("\n\r", pos);
        return str.substr(start + 1, pos - start);
    }
};
