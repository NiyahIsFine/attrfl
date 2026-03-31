#pragma once
#include <string_view>
#include <memory>
#include <string>
#include <vector>

class DataReader
{
public:
    virtual ~DataReader() = default;

    virtual bool IsValid() const = 0;

    virtual bool HasField(std::string_view key) const = 0;

    virtual int GetInt(std::string_view key, int def = 0) const = 0;

    virtual unsigned int GetUInt(std::string_view key, unsigned int def = 0) const = 0;

    virtual int64_t GetInt64(std::string_view key, int64_t def = 0) const = 0;

    virtual uint64_t GetUInt64(std::string_view key, uint64_t def = 0) const = 0;

    virtual float GetFloat(std::string_view key, float def = 0.f) const = 0;

    virtual double GetDouble(std::string_view key, double def = 0.f) const = 0;

    virtual bool GetBool(std::string_view key, bool def = false) const = 0;

    virtual std::string GetString(std::string_view key, std::string_view def = {}) const = 0;

    virtual std::unique_ptr<DataReader> GetObject(std::string_view key) const = 0;

    virtual int GetArraySize(std::string_view key) const = 0;

    virtual std::unique_ptr<DataReader> GetArrayElement(std::string_view key, int index) const = 0;

    virtual std::vector<std::string> GetMemberKeys() const = 0;
};
