/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetryHelper.h"
#include "TelemetrySerialiser.h"

#include <functional>

namespace Common::Telemetry
{
    void TelemetryHelper::set(const std::string& key, int value) { setInternal(key, value); }

    void TelemetryHelper::set(const std::string& key, unsigned int value) { setInternal(key, value); }

    void TelemetryHelper::set(const std::string& key, const std::string& value) { setInternal(key, value); }

    void TelemetryHelper::set(const std::string& key, const char* value) { setInternal(key, value); }

    void TelemetryHelper::set(const std::string& key, bool value) { setInternal(key, value); }
    
    void TelemetryHelper::increment(const std::string& key, int value) { incrementInternal(key, value); }

    void TelemetryHelper::increment(const std::string& key, unsigned int value) { incrementInternal(key, value); }
    
    void TelemetryHelper::appendValue(const std::string& arrayKey, int value) { appendValueInternal(arrayKey, value); }

    void TelemetryHelper::appendValue(const std::string& arrayKey, unsigned int value)
    {
        appendValueInternal(arrayKey, value);
    }

    void TelemetryHelper::appendValue(const std::string& arrayKey, const std::string& value)
    {
        appendValueInternal(arrayKey, value);
    }

    void TelemetryHelper::appendValue(const std::string& arrayKey, const char* value)
    {
        appendValueInternal(arrayKey, value);
    }

    void TelemetryHelper::appendValue(const std::string& arrayKey, bool value) { appendValueInternal(arrayKey, value); }
    
    void TelemetryHelper::appendObject(const std::string& arrayKey, const std::string& key, int value)
    {
        appendObjectInternal(arrayKey, key, value);
    }

    void TelemetryHelper::appendObject(const std::string& arrayKey, const std::string& key, unsigned int value)
    {
        appendObjectInternal(arrayKey, key, value);
    }

    void TelemetryHelper::appendObject(const std::string& arrayKey, const std::string& key, const std::string& value)
    {
        appendObjectInternal(arrayKey, key, value);
    }

    void TelemetryHelper::appendObject(const std::string& arrayKey, const std::string& key, const char* value)
    {
        appendObjectInternal(arrayKey, key, value);
    }

    void TelemetryHelper::appendObject(const std::string& arrayKey, const std::string& key, bool value)
    {
        appendObjectInternal(arrayKey, key, value);
    }

    TelemetryObject& TelemetryHelper::getTelemetryObjectByKey(const std::string& keyPath)
    {
        std::reference_wrapper<TelemetryObject> currentTelemObj = m_root;
        for (const auto& key : Common::UtilityImpl::StringUtils::splitString(keyPath, "."))
        {
            if (!currentTelemObj.get().keyExists(key))
            {
                currentTelemObj.get().set(key, TelemetryObject());
            }
            currentTelemObj = currentTelemObj.get().getObject(key);
        }
        return currentTelemObj;
    }

    std::string TelemetryHelper::serialise()
    {
        std::lock_guard<std::mutex> lock(m_dataLock);
        return TelemetrySerialiser::serialise(m_root);
    }

    void TelemetryHelper::registerResetCallback(std::string cookie, std::function<void()> function)
    {
        std::lock_guard<std::mutex> lock(m_callbackLock);
        if (m_callbacks.find(cookie) != m_callbacks.end())
        {
            throw std::logic_error("Callback already registered with cookie: " + cookie);
        }
        m_callbacks[cookie] = function;
    }

    void TelemetryHelper::unregisterResetCallback(std::string cookie)
    {
        std::lock_guard<std::mutex> lock(m_callbackLock);
        m_callbacks.erase(cookie);
    }

    void TelemetryHelper::reset()
    {
        clearData();
        std::lock_guard<std::mutex> callbackLock(m_callbackLock);
        for (const auto& callback_entry : m_callbacks)
        {
            if (callback_entry.second)
            {
                callback_entry.second();
            }
        }
    }

    void TelemetryHelper::clearData()
    {
        std::lock_guard<std::mutex> dataLock(m_dataLock);
        m_root = TelemetryObject();
    }

    void TelemetryHelper::mergeJsonIn(const std::string& key, const std::string& json)
    {
        std::lock_guard<std::mutex> dataLock(m_dataLock);
        TelemetryObject telemetryObject = TelemetrySerialiser::deserialise(json);
        getTelemetryObjectByKey(key) = telemetryObject;
    }

    std::string TelemetryHelper::serialiseAndReset()
    {
        std::scoped_lock lock(m_dataLock, m_callbackLock);

        // Serialise
        std::string serialised = TelemetrySerialiser::serialise(m_root);

        // Reset
        for (const auto& callback_entry : m_callbacks)
        {
            if (callback_entry.second)
            {
                callback_entry.second();
            }
        }
        m_root = TelemetryObject();

        return serialised;
    }


}