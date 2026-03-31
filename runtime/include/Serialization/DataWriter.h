#pragma once
#include <string_view>
#include <memory>
#include <string>

class DataWriter
{
public:
    virtual ~DataWriter() = default;

    virtual void WriteInt(std::string_view key, int val) = 0;

    virtual void WriteUInt(std::string_view key, unsigned int val) = 0;

    virtual void WriteInt64(std::string_view key, int64_t val) = 0;

    virtual void WriteUInt64(std::string_view key, uint64_t val) = 0;

    virtual void WriteFloat(std::string_view key, float val) = 0;

    virtual void WriteDouble(std::string_view key, double val) = 0;

    virtual void WriteBool(std::string_view key, bool val) = 0;

    virtual void WriteString(std::string_view key, std::string_view val) = 0;

    virtual void BeginObject(std::string_view key) = 0;

    virtual void EndObject() = 0;

    virtual void BeginArray(std::string_view key) = 0;

    virtual void EndArray() = 0;

    virtual std::string ToString() const = 0;

};

std::unique_ptr<DataWriter> CreateJsonDataWriter();
//std::unique_ptr<DataWriter> CreateXXXDataWriter(); ....