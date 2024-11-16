#pragma once
#include <memory>
#include <string>

struct TextInput {
    std::string content;
};

struct ImageInput {
    std::string path;
};

struct VideoInput {
    std::string path;
    int duration;
};

struct AudioInput {
    std::string path;
    int bitrate;
};

struct FileInput {
    std::string fileName;
};


// Базовый класс Data
class Data {
public:
    virtual ~Data() = default;

    std::string header;
    std::string body;
    int from = 0;
    int to = 0;
};

// Классы-наследники
class Text : public Data {
public:
    ~Text() override = default;
};

class Image final : public Data {
public:
    ~Image() override = default;
};

class Video : public Data {
public:
    ~Video() override = default;
};

class Audio final : public Data {
public:
    ~Audio() override = default;
};

class File : public Data {
public:
    ~File() override = default;
};

// Factory interface
class IDataFactory {
public:
    virtual ~IDataFactory() = default;

    // Factory methods
    virtual std::unique_ptr<Data> CreateText(const TextInput& input) const = 0;
    virtual std::unique_ptr<Data> CreateImage(const ImageInput& input) const = 0;
    virtual std::unique_ptr<Data> CreateVideo(const VideoInput& input) const = 0;
    virtual std::unique_ptr<Data> CreateAudio(const AudioInput& input) const = 0;
    virtual std::unique_ptr<Data> CreateFile(const FileInput& input) const = 0;
};

// Factory implementation
class DataFactory : public IDataFactory {
public:
    std::unique_ptr<Data> CreateText(const TextInput& input) const override {
        auto text = std::make_unique<Text>();
        text->header = "Text Header";
        text->body = !input.content.empty() ? input.content : "Default Text Content";
        return text;
    }

    std::unique_ptr<Data> CreateImage(const ImageInput& input) const override {
        auto image = std::make_unique<Image>();
        image->header = "Image Header";
        image->body = !input.path.empty() ? "Processed Image: " + input.path : "Default Image Path";
        return image;
    }

    std::unique_ptr<Data> CreateVideo(const VideoInput& input) const override {
        auto video = std::make_unique<Video>();
        video->header = "Video Header";
        video->body = !input.path.empty() ? "Processed Video: " + input.path : "Default Video Path";
        video->to = input.duration;
        return video;
    }

    std::unique_ptr<Data> CreateAudio(const AudioInput& input) const override {
        auto audio = std::make_unique<Audio>();
        audio->header = "Audio Header";
        audio->body = !input.path.empty() ? "Processed Audio: " + input.path : "Default Audio Path";
        audio->to = input.bitrate;
        return audio;
    }

    std::unique_ptr<Data> CreateFile(const FileInput& input) const override {
        auto file = std::make_unique<File>();
        file->header = "File Header";
        file->body = !input.fileName.empty() ? "Processed File: " + input.fileName : "Default File Name";
        return file;
    }
};
