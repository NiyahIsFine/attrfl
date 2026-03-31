#include "JsonDataWriter.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4996)
#endif
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

JsonDataWriter::JsonDataWriter()
{
    m_doc.SetObject();
    static constexpr size_t ExpectedMaxInheritanceDepth = 8;
    m_objectStack.reserve(ExpectedMaxInheritanceDepth);
}

static rapidjson::Value MakeKey(std::string_view key, rapidjson::Document::AllocatorType &alloc)
{
    return rapidjson::Value(key.data(), static_cast<rapidjson::SizeType>(key.size()), alloc);
}

void JsonDataWriter::WriteValue(std::string_view key, rapidjson::Value val)
{
    if (!m_objectStack.empty() && m_objectStack.back().isArray)
    {
        m_objectStack.back().obj.PushBack(std::move(val), m_doc.GetAllocator());
    }
    else
    {
        rapidjson::Value k = MakeKey(key, m_doc.GetAllocator());
        if (m_objectStack.empty())
            m_doc.AddMember(k, std::move(val), m_doc.GetAllocator());
        else
            m_objectStack.back().obj.AddMember(k, std::move(val), m_doc.GetAllocator());
    }
}

void JsonDataWriter::PopAndMerge()
{
    if (m_objectStack.empty()) return;

    auto &entry = m_objectStack.back();

    if (m_objectStack.size() == 1)
    {
        rapidjson::Value k = MakeKey(entry.key, m_doc.GetAllocator());
        m_doc.AddMember(k, std::move(entry.obj), m_doc.GetAllocator());
    }
    else
    {
        auto &parentEntry = m_objectStack[m_objectStack.size() - 2];
        if (parentEntry.isArray)
        {
            parentEntry.obj.PushBack(std::move(entry.obj), m_doc.GetAllocator());
        }
        else
        {
            rapidjson::Value k = MakeKey(entry.key, m_doc.GetAllocator());
            parentEntry.obj.AddMember(k, std::move(entry.obj), m_doc.GetAllocator());
        }
    }

    m_objectStack.pop_back();
}

void JsonDataWriter::WriteInt(std::string_view key, int val)
{
    WriteValue(key, rapidjson::Value(val));
}

void JsonDataWriter::WriteUInt(std::string_view key, unsigned int val)
{
    WriteValue(key, rapidjson::Value(val));
}

void JsonDataWriter::WriteInt64(std::string_view key, int64_t val)
{
    WriteValue(key, rapidjson::Value(val));
}

void JsonDataWriter::WriteUInt64(std::string_view key, uint64_t val)
{
    WriteValue(key, rapidjson::Value(val));
}

void JsonDataWriter::WriteFloat(std::string_view key, float val)
{
    WriteValue(key, rapidjson::Value(val));
}

void JsonDataWriter::WriteDouble(std::string_view key, double val)
{
    WriteValue(key, rapidjson::Value(val));
}

void JsonDataWriter::WriteBool(std::string_view key, bool val)
{
    WriteValue(key, rapidjson::Value(val));
}

void JsonDataWriter::WriteString(std::string_view key, std::string_view val)
{
    rapidjson::Value v(val.data(), static_cast<rapidjson::SizeType>(val.size()), m_doc.GetAllocator());
    WriteValue(key, std::move(v));
}

void JsonDataWriter::BeginObject(std::string_view key)
{
    m_objectStack.emplace_back(key, false);
}

void JsonDataWriter::EndObject()
{
    PopAndMerge();
}

void JsonDataWriter::BeginArray(std::string_view key)
{
    m_objectStack.emplace_back(key, true);
}

void JsonDataWriter::EndArray()
{
    PopAndMerge();
}

std::string JsonDataWriter::ToString() const
{
    rapidjson::StringBuffer sb;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
    m_doc.Accept(writer);
    return sb.GetString();
}

std::unique_ptr<DataWriter> CreateJsonDataWriter()
{
    return std::make_unique<JsonDataWriter>();
}