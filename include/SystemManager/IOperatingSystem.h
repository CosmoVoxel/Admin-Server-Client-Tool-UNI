#pragma once
#include <fstream>
#include <iostream>


class OperatingSystemManager final
{
public:
    static CmdResult_S RunCMDCommand(const std::string& command)
    {
        // Open a pipe to the command
        CmdResult_S cmd_result;
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe)
        {
            std::cerr << "Error executing command. Pipe error" << std::endl;
            cmd_result.result = "Error executing command. Pipe error";
            return cmd_result;
        }

        // Read the output from the command
        char buffer[128];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe) != NULL)
        {
            result += buffer; // Append the output to result
        }

        // Close the pipe
        fclose(pipe);

        cmd_result.result = result;
        return cmd_result;
    }
};
