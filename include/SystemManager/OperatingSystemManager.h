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

private:
};
