/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "TelemetryObject.h"

#include <functional>
#include <mutex>
#include <string>

namespace Common::Telemetry
{
    class TelemetryHelper
    {
    public:
        static TelemetryHelper& getInstance()
        {
            static TelemetryHelper instance;
            return instance;
        }

        TelemetryHelper(TelemetryHelper const&) = delete;

        void operator=(TelemetryHelper const&) = delete;

        void set(const std::string& key, long value);
        void set(const std::string& key, unsigned long value);
        void set(const std::string& key, double value);
        void set(const std::string& key, const std::string& value);
        void set(const std::string& key, const char* value);
        void set(const std::string& key, bool value);
        void set(const std::string& key, const TelemetryObject & object, bool stick);

        void increment(const std::string& key, long value);
        void increment(const std::string& key, unsigned long value);

        void appendValue(const std::string& arrayKey, long value);
        void appendValue(const std::string& arrayKey, unsigned long value);
        void appendValue(const std::string& arrayKey, double value);
        void appendValue(const std::string& arrayKey, const std::string& value);
        void appendValue(const std::string& arrayKey, const char* value);
        void appendValue(const std::string& arrayKey, bool value);

        void appendObject(const std::string& arrayKey, const TelemetryObject& object);
        void appendObject(const std::string& arrayKey, const std::string& key, long value);
        void appendObject(const std::string& arrayKey, const std::string& key, unsigned long value);
        void appendObject(const std::string& arrayKey, const std::string& key, double value);
        void appendObject(const std::string& arrayKey, const std::string& key, const std::string& value);
        void appendObject(const std::string& arrayKey, const std::string& key, const char* value);
        void appendObject(const std::string& arrayKey, const std::string& key, bool value);

        void appendStat(const std::string& statsKey, double value);
        void updateTelemetryWithStats();

        double getStatAverage(const std::string& statsKey);
        double getStatMin(const std::string& statsKey);
        double getStatMax(const std::string& statsKey);
        void updateTelemetryWithAllAverageStats();
        void updateTelemetryWithAllMinStats();
        void updateTelemetryWithAllMaxStats();

        void mergeJsonIn(const std::string& key, const std::string& json);
        void registerResetCallback(std::string cookie, std::function<void(TelemetryHelper&)> function);
        void unregisterResetCallback(std::string cookie);
        void reset();
        std::string serialise();
        std::string serialiseAndReset();

        // Normally with a singleton the constructor is private but here we make the constructor public
        // so that plugins can instantiate multiple Telemetry Helpers and not share a root data structure if they want
        // to.
        TelemetryHelper() = default;

    private:
        TelemetryObject m_root;
        TelemetryObject m_resetToThis;
        std::mutex m_dataLock;
        std::mutex m_callbackLock;
        std::map<std::string, std::function<void(TelemetryHelper&)>> m_callbacks;
        std::map<std::string, std::vector<double>> m_statsCollection;

        void locked_reset();

        template<class T>
        void setInternal(const std::string& key, T value, bool stick)
        {
            std::lock_guard<std::mutex> lock(m_dataLock);
            TelemetryValue telemetryValue(value);
            getTelemetryObjectByKey(key).set(telemetryValue);
            if ( stick )
            {
                getTelemetryObjectByKey(key,m_resetToThis).set(telemetryValue);
            }
        }

        template<class T>
        void incrementInternal(const std::string& key, T value)
        {
            std::lock_guard<std::mutex> lock(m_dataLock);
            TelemetryObject& telemetryObject = getTelemetryObjectByKey(key);
            TelemetryValue telemetryValue(0L);

            if (telemetryObject.getType() != TelemetryObject::Type::value)
            {
                telemetryObject.set(telemetryValue);
            }

            TelemetryValue::Type valueType = telemetryObject.getValue().getType();
            if (valueType == TelemetryValue::Type::integer_type)
            {
                long newValue = telemetryObject.getValue().getInteger() + value;
                telemetryValue.set(newValue);
            }
            else if (valueType == TelemetryValue::Type::unsigned_integer_type)
            {
                unsigned long newValue = telemetryObject.getValue().getUnsignedInteger() + value;
                telemetryValue.set(newValue);
            }

            telemetryObject.set(telemetryValue);
        }

        template<class T>
        void appendValueInternal(const std::string& key, T value)
        {
            std::lock_guard<std::mutex> lock(m_dataLock);
            TelemetryObject& telemetryObject = getTelemetryObjectByKey(key);

            // Force telemetry object to be an array, they are defaulted to object type.
            if (telemetryObject.getType() != TelemetryObject::Type::array)
            {
                telemetryObject.set(std::list<TelemetryObject>());
            }

            std::list<TelemetryObject>& list = telemetryObject.getArray();
            TelemetryValue newValue(value);
            TelemetryObject newObj;
            newObj.set(newValue);
            list.emplace_back(newObj);
        }

        template<class T>
        void appendObjectInternal(const std::string& arrayKey, const std::string& key, T value)
        {
            std::lock_guard<std::mutex> lock(m_dataLock);
            TelemetryObject& telemetryObject = getTelemetryObjectByKey(arrayKey);

            // Force telemetry object to be an array, they are defaulted to object type.
            if (telemetryObject.getType() != TelemetryObject::Type::array)
            {
                telemetryObject.set(std::list<TelemetryObject>());
            }

            std::list<TelemetryObject>& list = telemetryObject.getArray();
            TelemetryValue newValue(value);
            TelemetryObject newObj;
            newObj.set(key, newValue);
            list.emplace_back(newObj);
        }

        TelemetryObject& getTelemetryObjectByKey(const std::string& keyPath);
        TelemetryObject& getTelemetryObjectByKey(const std::string& keyPath, TelemetryObject & root);
        void clearData();
    };
} // namespace Common::Telemetry