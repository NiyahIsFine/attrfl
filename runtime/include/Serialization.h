#pragma once
#include "Reflection/TypeInfo.h"
#include "Reflection/FieldInfo.h"
#include "Serialization/DataReader.h"
#include "Serialization/DataWriter.h"

namespace attrfl
{
    class Serialization
    {
    public:
        static void ApplyFieldFromReader(void *obj, const FieldInfo &field, const DataReader &reader);

        static void WriteFieldToWriter(const void *obj, const FieldInfo &field, DataWriter &writer);

        static void WritePrimitiveValue(const void *valPtr, const TypeInfo *type, DataWriter &writer,
                                        std::string_view key = "");

        static void ReadPrimitiveValue(void *valPtr, const TypeInfo *type, const DataReader &reader,
                                       std::string_view key = "");

        static void WriteNestedSerializable(const void *fieldPtr, const TypeInfo *type, DataWriter &writer,
                                            std::string_view key);

        static void ReadNestedSerializable(void *fieldPtr, const TypeInfo *type, const DataReader &reader,
                                           std::string_view key);
    };
}
