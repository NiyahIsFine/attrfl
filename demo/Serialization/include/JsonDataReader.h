#pragma once

#include "Serialization/DataReader.h"
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4996)
#endif
#include "rapidjson/document.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

class JsonDataReader final : public DataReader
{
public:
    explicit JsonDataReader(const rapidjson::Value &root);

    bool IsValid() const override;

    bool HasField(std::string_view key) const override;

    int GetInt(std::string_view key, int def) const override;

    unsigned int GetUInt(std::string_view key,  unsigned int def) const override;

    int64_t GetInt64(std::string_view key, int64_t def) const override;

    uint64_t GetUInt64(std::string_view key, uint64_t def) const override;

    float GetFloat(std::string_view key, float def) const override;

    double GetDouble(std::string_view key, double def = 0.f) const override;

    bool GetBool(std::string_view key, bool def) const override;

    std::string GetString(std::string_view key, std::string_view def) const override;

    std::unique_ptr<DataReader> GetObject(std::string_view key) const override;

    int GetArraySize(std::string_view key) const override;

    std::unique_ptr<DataReader> GetArrayElement(std::string_view key, int index) const override;

    std::vector<std::string> GetMemberKeys() const override;

private:
    const rapidjson::Value *FindMember(std::string_view key) const;

    const rapidjson::Value *FindValueOrRoot(std::string_view key) const;

    const rapidjson::Value &m_root; // *
};

std::unique_ptr<DataReader> CreateJsonDataReader(rapidjson::Document& document);