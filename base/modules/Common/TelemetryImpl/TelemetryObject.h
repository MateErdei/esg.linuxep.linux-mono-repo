/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <list>
#include <map>
#include <memory>
#include <string>
#include "TelemetryValue.h"

namespace Common::Telemetry
{

    class TelemetryObject
    {
    public:

        enum class Type
        {
            array,
            object,
            value
        };

        TelemetryObject();

//        void set(std::string key, const char* value);
//        void set(std::string key, std::string value);
//        void set(std::string key, bool value);
//        void set(std::string key, int value);
        void set(std::string key, TelemetryValue value);
        void set(std::string key, TelemetryObject value);
        void set(std::string key, std::list<TelemetryObject> objectList);
        void set(const char* value);

//        void set(std::string value);
//        void set(int value);
//        void set(bool value);
        void set(TelemetryValue value);
        void set(std::list<TelemetryObject> value);

//        std::string getString();
//        int getInt();
//        bool getBool();
        TelemetryValue getValue() const;
        std::list<TelemetryObject> getArray() const;
        TelemetryObject getObject(std::string key);
        const std::map<std::string, TelemetryObject>& getChildObjects() const;

        Type getType() const;
//        TelemetryValue::ValueType getValueType();
        bool keyExists(std::string key);

        bool operator==(const TelemetryObject& rhs) const;

        bool operator!=(const TelemetryObject& rhs) const;

    private:

        void checkType(Type expectedType) const;

        Type m_type;
        std::map<std::string, TelemetryObject> m_nodes;
        std::list<TelemetryObject> m_array;
        TelemetryValue m_value;
    };
}