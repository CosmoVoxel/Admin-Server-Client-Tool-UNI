#pragma once
#include <json/json.hpp>

class DataStruct
{
public:
    virtual ~DataStruct() = default;
};

struct CmdCommand_S final : public DataStruct
{
    std::string command;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(CmdCommand_S, command);
};

struct CmdResult_S final : public DataStruct
{
    std::string result;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(CmdResult_S, result);
};




