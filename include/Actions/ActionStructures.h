#pragma once
#include <json/json.hpp>

class DataStruct
{
public:
    virtual ~DataStruct() = default;
};

// ----=== CMD Commands STR ===----

struct CmdCommand_S final : public DataStruct
{
    std::string command;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(CmdCommand_S, command);
    CmdCommand_S() = default;
    explicit CmdCommand_S(const char* text)
    {
        command = text;
    };
};

struct CmdResult_S final : public DataStruct
{
    std::string result;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(CmdResult_S, result);
};

// ----=== PC Status STR ===----
/// \brief Structure representing the status of a PC.
/// \details Contains information about the PC's IP address, MAC address, and operating system.
struct PCStatus_S_OUT final : public DataStruct
{
    /// \brief IP address of the PC.
    std::string ip;

    /// \brief MAC address of the PC.
    std::string mac;

    /// \brief Operating system of the PC.
    std::string os;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(PCStatus_S_OUT, ip, mac, os);
};

struct IsClientUp_S final : DataStruct
{
    bool is_up;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(IsClientUp_S, is_up);
    explicit IsClientUp_S(const bool is_up) : is_up(is_up) {};
    explicit IsClientUp_S() = default;
};

struct BasicDebugMessageS : public DataStruct
{

};

enum ClientIdErrorType
{
    Incorrect = 0,
    Ok = 1
};

struct ErrorMessageSendingClientIdS final : public BasicDebugMessageS
{
    ClientIdErrorType error_type;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ErrorMessageSendingClientIdS, error_type);
    explicit ErrorMessageSendingClientIdS(const ClientIdErrorType error_type) : error_type(error_type) {};
};
