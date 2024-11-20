#pragma once
#include <memory>
#include <string>
#include "json.hpp"

using json= nlohmann::json;
struct DataInput {

};

// Базовый класс Data
class Data {
public:
    virtual ~Data() = default;
};


class Text final : public Data {
public:
    ~Text() override = default;
    std::string text;
};

class Position final : public Data {
public:
    ~Position() override = default;
    int x{};
    int y{};
};

class File final : public Data {
public:
    ~File() override = default;
    std::string path;
};

// Macro to DATA serialization/deserialization.
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Position, x, y);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Text, text)



// Factory implementation
class DataFactory{
public:
    static Data& CreateText(const std::string& text){
        auto text_data = Text();
        text_data.text = text;
        return text_data;
    }

    static Data& CreatePosition(const int x, const int y){
        auto position = Position();
        position.x = x;
        position.y = y;
        return position;
    }

    static Data& CreateFile(const std::string& path){
        auto file = File();
        file.path = path;
        return file;
    }
};


