#include "BaseSO.h"
#include "Serialization.h"
#include "JsonDataReader.h"
#include "JsonDataWriter.h"
#include <string>
#include <filesystem>

using namespace attrfl;
BaseSO::BaseSO() : baseValue(1), baseValue2(1), baseValue3(1)
{
}


void BaseSO::Load(std::filesystem::path filePath)
{
    FILE *file = nullptr;

#ifdef _WIN32
    auto errNo = _wfopen_s(&file, filePath.c_str(), L"rb");
    if (errNo != 0)
    {
        return;
    }
#else
    file = fopen(resolvedPath.c_str(), "rb");
#endif
    if (!file)
    {
        return;
    }
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (size < 0)
    {
        fclose(file);
        return;
    }

    std::string buffer;
    buffer.resize(static_cast<size_t>(size));

    size_t readBytes = fread(buffer.data(), 1, size, file);
    fclose(file);

    if (readBytes != static_cast<size_t>(size))
    {
        return;
    }
    rapidjson::Document document;
    document.Parse(buffer.data(), buffer.size());
    auto reader = CreateJsonDataReader(document);
    if (reader && reader->IsValid())
    {
        TypeInfo *typeInfo = GetType();
        if (typeInfo)
        {
            const TypeInfo *current = typeInfo;
            while (current)
            {
                auto classReader = reader->GetObject(current->GetName());
                if (classReader && classReader->IsValid())
                {
                    for (const auto &field : current->GetFields())
                    {
                        if (!field.HasAttribute("EPSerializable")) continue;
                        if (!classReader->HasField(field.name)) continue;
                        Serialization::ApplyFieldFromReader(this, field, *classReader);
                    }
                }
                current = current->GetParent();
            }
        }
    }
}

void BaseSO::Save(std::filesystem::path filePath)
{
    TypeInfo *typeInfo = GetType();
    std::string jsonBody = "{}";

    if (typeInfo)
    {
        auto writer = CreateJsonDataWriter();

        const TypeInfo *current = typeInfo;
        while (current)
        {
            const auto &ownFields = current->GetFields();

            bool hasSerializable = false;
            for (const auto &field : ownFields)
            {
                if (field.HasAttribute("EPSerializable")) { hasSerializable = true; break; }
            }

            if (hasSerializable)
            {
                writer->BeginObject(current->GetName());
                for (const auto &field : ownFields)
                {
                    if (!field.HasAttribute("EPSerializable")) continue;
                    Serialization::WriteFieldToWriter(this, field, *writer);
                }
                writer->EndObject();
            }
            current = current->GetParent();
        }

        jsonBody = writer->ToString();
    }

    FILE *file = nullptr;
#ifdef _WIN32
    auto errNo = _wfopen_s(&file, filePath.c_str(), L"wb");
    if (errNo != 0)
    {
        return;
    }
#else
    file = fopen(resolvedPath.c_str(), "wb");
#endif
    if (!file)
    {
        return;
    }

    size_t written = fwrite(jsonBody.data(), 1, jsonBody.size(), file);
    fclose(file);
}
