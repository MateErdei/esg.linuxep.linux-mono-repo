/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "TelemetryValue.h"
#include <vector>
#include <list>
#include <map>
#include <string>
#include <variant>

namespace Common::Telemetry
{
    class TelemetryObject
    {
    public:
        // helper function to reduce the boiler plate code and allow for
        // TelemetryObject v = fromVectorOfKeyValues( { {"key1","value1"}, {"key2","value2"}});
        static TelemetryObject fromVectorOfKeyValues( const std::vector<std::pair<std::string, std::string>>&  );


        // This must be in the same order as the variant types of m_value.
        enum class Type
        {
            value,
            object,
            array
        };

        TelemetryObject();

        void set(const std::string& key, const TelemetryValue& value);
        void set(const std::string& key, const TelemetryObject& value);
        void set(const std::string& key, const std::list<TelemetryObject>& objectList);
        void set(const TelemetryValue& value);
        void set(const std::list<TelemetryObject>& value);

        TelemetryObject& getObject(const std::string& key);

        TelemetryValue& getValue();
        const TelemetryValue& getValue() const;

        std::list<TelemetryObject>& getArray();
        const std::list<TelemetryObject>& getArray() const;

        std::map<std::string, TelemetryObject>& getChildObjects();
        const std::map<std::string, TelemetryObject>& getChildObjects() const;

        Type getType() const;
        bool keyExists(const std::string& key) const;

        bool operator==(const TelemetryObject& rhs) const;
        bool operator!=(const TelemetryObject& rhs) const;

    private:
        void checkType(Type expectedType) const;

        // Type m_type;
        std::variant<TelemetryValue, std::map<std::string, TelemetryObject>, std::list<TelemetryObject>> m_value;
    };
} // namespace Common::Telemetry