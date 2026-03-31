#include "Serialization.h"
#include "Reflection/Type.h"
#include <string>
#include <memory>

using namespace attrfl;

void Serialization::ApplyFieldFromReader(void *obj, const FieldInfo &field, const DataReader &reader)
{
    ReadPrimitiveValue(field.GetRawPtr(obj), field.type, reader, field.name);
}

void Serialization::WriteFieldToWriter(const void *obj, const FieldInfo &field, DataWriter &writer)
{
    WritePrimitiveValue(field.GetRawPtr(obj), field.type, writer, field.name);
}

void Serialization::WritePrimitiveValue(const void *valPtr, const TypeInfo *type, DataWriter &writer,
                                        std::string_view key)
{
    if (!valPtr || !type)
        return;

    if (type == Type<int>::Get()) writer.WriteInt(key, *static_cast<const int *>(valPtr));
    else if (type == Type<unsigned int>::Get()) writer.WriteUInt(key, *static_cast<const unsigned int *>(valPtr));
    else if (type == Type<int64_t>::Get()) writer.WriteInt64(key, *static_cast<const int64_t *>(valPtr));
    else if (type == Type<uint64_t>::Get()) writer.WriteUInt64(key, *static_cast<const uint64_t *>(valPtr));
    else if (type == Type<float>::Get()) writer.WriteFloat(key, *static_cast<const float *>(valPtr));
    else if (type == Type<double>::Get()) writer.WriteDouble(key, *static_cast<const double *>(valPtr));
    else if (type == Type<bool>::Get()) writer.WriteBool(key, *static_cast<const bool *>(valPtr));
    else if (type == Type<std::string>::Get()) writer.WriteString(key, *static_cast<const std::string *>(valPtr));
    else if (type->IsList() && type->listGetSize && type->listGetElement)
    {
        writer.BeginArray(key);
        size_t sz = type->listGetSize(valPtr);
        for (size_t i = 0; i < sz; ++i)
        {
            const void *elemPtr = type->listGetElement(valPtr, i);
            WritePrimitiveValue(elemPtr, type->valueType, writer, "");
        }
        writer.EndArray();
    }
    else if (type->IsDict() && type->dictForEach)
    {
        writer.BeginObject(key);
        type->dictForEach(valPtr, [&](const std::string &dictKey, const void *valP)
        {
            WritePrimitiveValue(valP, type->valueType, writer, dictKey);
        });
        writer.EndObject();
    }
    else if (type->HasAttribute("EPClassSerializable"))
    {
        WriteNestedSerializable(valPtr, type, writer, key);
    }
}

void Serialization::ReadPrimitiveValue(void *valPtr, const TypeInfo *type, const DataReader &reader,
                                       std::string_view key)
{
    if (!valPtr || !type)
        return;

    if (type == Type<int>::Get()) *static_cast<int *>(valPtr) = reader.GetInt(key);
    else if (type == Type<unsigned int>::Get()) *static_cast<unsigned int *>(valPtr) = reader.GetUInt(key);
    else if (type == Type<int64_t>::Get()) *static_cast<int64_t *>(valPtr) = reader.GetInt64(key);
    else if (type == Type<uint64_t>::Get()) *static_cast<uint64_t *>(valPtr) = reader.GetUInt64(key);
    else if (type == Type<float>::Get()) *static_cast<float *>(valPtr) = reader.GetFloat(key);
    else if (type == Type<double>::Get()) *static_cast<double *>(valPtr) = reader.GetDouble(key);
    else if (type == Type<bool>::Get()) *static_cast<bool *>(valPtr) = reader.GetBool(key);
    else if (type == Type<std::string>::Get()) *static_cast<std::string *>(valPtr) = reader.GetString(key);

    else if (type->IsList() && type->listClear && type->listAddDefault)
    {
        int arraySize = reader.GetArraySize(key);
        type->listClear(valPtr);
        for (int i = 0; i < arraySize; ++i)
        {
            void *elemPtr = type->listAddDefault(valPtr);
            auto elemReader = reader.GetArrayElement(key, i);
            if (elemReader)
                ReadPrimitiveValue(elemPtr, type->valueType, *elemReader, "");
        }
    }
    else if (type->IsDict() && type->dictClear && type->dictInsert)
    {
        std::unique_ptr<DataReader> subReaderOwned;
        const DataReader *subReader = key.empty()
                                          ? &reader
                                          : (subReaderOwned = reader.GetObject(key)).get();
        if (!subReader) return;

        type->dictClear(valPtr);
        for (const auto &memberKey: subReader->GetMemberKeys())
        {
            void *valP = type->dictInsert(valPtr, memberKey);
            ReadPrimitiveValue(valP, type->valueType, *subReader, memberKey);
        }
    }
    else if (type->HasAttribute("EPClassSerializable"))
    {
        ReadNestedSerializable(valPtr, type, reader, key);
    }
}

void Serialization::WriteNestedSerializable(const void *fieldPtr, const TypeInfo *t, DataWriter &writer,
                                            std::string_view key)
{
    writer.BeginObject(key);
    const TypeInfo *nested = t;
    while (nested)
    {
        const auto &ownFields = nested->GetFields();
        bool hasSerializable = false;
        for (const auto &subField: ownFields)
        {
            if (subField.HasAttribute("EPSerializable"))
            {
                hasSerializable = true;
                break;
            }
        }
        if (hasSerializable)
        {
            writer.BeginObject(nested->GetName());
            for (const auto &subField: ownFields)
            {
                if (!subField.HasAttribute("EPSerializable")) continue;
                WriteFieldToWriter(fieldPtr, subField, writer);
            }
            writer.EndObject();
        }
        nested = nested->GetParent();
    }
    writer.EndObject();
}

void Serialization::ReadNestedSerializable(void *fieldPtr, const TypeInfo *t, const DataReader &reader,
                                           std::string_view key)
{
    auto subReader = reader.GetObject(key);
    if (!subReader || !subReader->IsValid())
        return;

    const TypeInfo *nested = t;
    while (nested)
    {
        auto classReader = subReader->GetObject(nested->GetName());
        if (classReader && classReader->IsValid())
        {
            for (const auto &subField: nested->GetFields())
            {
                if (!subField.HasAttribute("EPSerializable")) continue;
                if (!classReader->HasField(subField.name)) continue;
                ApplyFieldFromReader(fieldPtr, subField, *classReader);
            }
        }
        nested = nested->GetParent();
    }
}
