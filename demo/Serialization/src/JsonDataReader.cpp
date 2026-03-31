#include "JsonDataReader.h"

#include <filesystem>

JsonDataReader::JsonDataReader(const rapidjson::Value &root)
    : m_root(root)
{
}

const rapidjson::Value *JsonDataReader::FindMember(std::string_view key) const
{
    if (!m_root.IsObject()) return nullptr;
    auto it = m_root.FindMember(rapidjson::StringRef(key.data(), key.size()));
    if (it == m_root.MemberEnd()) return nullptr;
    return &it->value;
}

const rapidjson::Value *JsonDataReader::FindValueOrRoot(std::string_view key) const
{
    if (key.empty()) return &m_root;
    return FindMember(key);
}

bool JsonDataReader::IsValid() const
{
    return m_root.IsObject();
}

bool JsonDataReader::HasField(std::string_view key) const
{
    return FindMember(key) != nullptr;
}

int JsonDataReader::GetInt(std::string_view key, int def) const
{
    const auto *v = FindValueOrRoot(key);
    return (v && v->IsInt()) ? v->GetInt() : def;
}

unsigned int JsonDataReader::GetUInt(std::string_view key, unsigned int def) const
{
    const auto *v = FindValueOrRoot(key);
    return (v && v->IsUint()) ? v->GetUint() : def;
}

int64_t JsonDataReader::GetInt64(std::string_view key, int64_t def) const
{
    const auto *v = FindValueOrRoot(key);
    return (v && v->IsInt64()) ? v->GetInt64() : def;
}

uint64_t JsonDataReader::GetUInt64(std::string_view key, uint64_t def) const
{
    const auto *v = FindValueOrRoot(key);
    return (v && v->IsUint64()) ? v->GetUint64() : def;
}

float JsonDataReader::GetFloat(std::string_view key, float def) const
{
    const auto *v = FindValueOrRoot(key);
    if (!v) return def;
    if (v->IsFloat()) return v->GetFloat();
    if (v->IsDouble()) return static_cast<float>(v->GetDouble());
    if (v->IsInt()) return static_cast<float>(v->GetInt());
    return def;
}

double JsonDataReader::GetDouble(std::string_view key, double def) const
{
    const auto *v = FindValueOrRoot(key);
    if (!v) return def;
    if (v->IsFloat()) return static_cast<double>(v->GetFloat());
    if (v->IsDouble()) return v->GetDouble();
    if (v->IsInt()) return static_cast<double>(v->GetInt());
    return def;
}


bool JsonDataReader::GetBool(std::string_view key, bool def) const
{
    const auto *v = FindValueOrRoot(key);
    return (v && v->IsBool()) ? v->GetBool() : def;
}

std::string JsonDataReader::GetString(std::string_view key, std::string_view def) const
{
    const auto *v = FindValueOrRoot(key);
    return (v && v->IsString())
               ? std::string(v->GetString(), v->GetStringLength())
               : std::string(def);
}

std::unique_ptr<DataReader> JsonDataReader::GetObject(std::string_view key) const
{
    const auto *v = FindValueOrRoot(key);
    if (!v || !v->IsObject()) return nullptr;
    return std::make_unique<JsonDataReader>(*v);
}

int JsonDataReader::GetArraySize(std::string_view key) const
{
    const auto *v = FindValueOrRoot(key);
    if (!v || !v->IsArray()) return 0;
    return static_cast<int>(v->GetArray().Size());
}

std::unique_ptr<DataReader> JsonDataReader::GetArrayElement(std::string_view key, int index) const
{
    const auto *v = FindValueOrRoot(key);
    if (!v || !v->IsArray()) return nullptr;
    const auto &jsonArray = v->GetArray();
    if (index < 0 || index >= static_cast<int>(jsonArray.Size())) return nullptr;
    return std::make_unique<JsonDataReader>(jsonArray[index]);
}

std::vector<std::string> JsonDataReader::GetMemberKeys() const
{
    std::vector<std::string> keys;
    if (!m_root.IsObject()) return keys;
    for (auto it = m_root.MemberBegin(); it != m_root.MemberEnd(); ++it)
        keys.emplace_back(it->name.GetString(), it->name.GetStringLength());
    return keys;
}

std::unique_ptr<DataReader> CreateJsonDataReader(rapidjson::Document& document)
{
    return std::make_unique<JsonDataReader>(document);
}