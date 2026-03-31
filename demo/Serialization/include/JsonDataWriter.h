#pragma once

#include "Serialization/DataWriter.h"
#include <vector>
#include <string>
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4996)
#endif
#include "rapidjson/document.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

class JsonDataWriter final : public DataWriter
{
public:
    JsonDataWriter();

    void WriteInt(std::string_view key, int val) override;

    void WriteUInt(std::string_view key, unsigned int val) override;

    void WriteInt64(std::string_view key, int64_t val) override;

    void WriteUInt64(std::string_view key, uint64_t val) override;

    void WriteFloat(std::string_view key, float val) override;

    void WriteDouble(std::string_view key, double val) override;

    void WriteBool(std::string_view key, bool val) override;

    void WriteString(std::string_view key, std::string_view val) override;

    void BeginObject(std::string_view key) override;

    void EndObject() override;

    void BeginArray(std::string_view key) override;

    void EndArray() override;

    std::string ToString() const override;

private:
    void WriteValue(std::string_view key, rapidjson::Value val);

    void PopAndMerge();

    rapidjson::Document m_doc;

    struct NestedEntry
    {
        std::string key;
        rapidjson::Value obj;
        bool isArray; // true = 数组，false = 对象

        NestedEntry(std::string_view k, bool arr)
            : key(k)
            , obj(arr ? rapidjson::kArrayType : rapidjson::kObjectType)
            , isArray(arr)
        {}
    };
    std::vector<NestedEntry> m_objectStack;
};
