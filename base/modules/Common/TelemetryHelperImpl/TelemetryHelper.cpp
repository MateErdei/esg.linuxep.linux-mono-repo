/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetryHelper.h"

#include "TelemetrySerialiser.h"

#include <Common/UtilityImpl/StringUtils.h>

#include <functional>

namespace Common::Telemetry
{
    void TelemetryHelper::set(const std::string& key, long value) { setInternal(key, value, false); }

    void TelemetryHelper::set(const std::string& key, unsigned long value) { setInternal(key, value, false); }
    void TelemetryHelper::set(const std::string& key, const std::string& value) { setInternal(key, value, false); }

    void TelemetryHelper::set(const std::string& key, const char* value) { setInternal(key, value, false); }

    void TelemetryHelper::set(const std::string& key, bool value) { setInternal(key, value, false); }

    void TelemetryHelper::increment(const std::string& key, long value) { incrementInternal(key, value); }

    void TelemetryHelper::increment(const std::string& key, unsigned long value) { incrementInternal(key, value); }

    void TelemetryHelper::appendValue(const std::string& arrayKey, long value) { appendValueInternal(arrayKey, value); }

    void TelemetryHelper::appendValue(const std::string& arrayKey, unsigned long value)
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

    void TelemetryHelper::appendObject(const std::string& arrayKey, const std::string& key, long value)
    {
        appendObjectInternal(arrayKey, key, value);
    }

    void TelemetryHelper::appendObject(const std::string& arrayKey, const TelemetryObject& newObject)
    {
        std::lock_guard<std::mutex> lock(m_dataLock);
        TelemetryObject& telemetryObject = getTelemetryObjectByKey(arrayKey);

        // Force telemetry object to be an array, they are defaulted to object type.
        if (telemetryObject.getType() != TelemetryObject::Type::array)
        {
            telemetryObject.set(std::list<TelemetryObject>());
        }

        std::list<TelemetryObject>& list = telemetryObject.getArray();
        list.emplace_back(newObject);
    }

    void TelemetryHelper::appendObject(const std::string& arrayKey, const std::string& key, unsigned long value)
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
        return getTelemetryObjectByKey(keyPath, m_root);
    }

    TelemetryObject& TelemetryHelper::getTelemetryObjectByKey(const std::string& keyPath, TelemetryObject& root)
    {
        std::reference_wrapper<TelemetryObject> currentTelemObj = root;
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

    void TelemetryHelper::registerResetCallback(std::string cookie, std::function<void(TelemetryHelper&)> function)
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
        std::scoped_lock scopedLock( m_callbackLock, m_dataLock);
        locked_reset();
    }
    void TelemetryHelper::locked_reset()
    {
        TelemetryHelper another;
        another.m_root = m_resetToThis;
        for (const auto& callback_entry : m_callbacks)
        {
            if (callback_entry.second)
            {
                callback_entry.second(another);
            }
        }
        m_root = another.m_root;

    }

    void TelemetryHelper::clearData()
    {
        std::lock_guard<std::mutex> dataLock(m_dataLock);
        m_root = m_resetToThis;
    }

    void TelemetryHelper::mergeJsonIn(const std::string& key, const std::string& json)
    {
        std::lock_guard<std::mutex> dataLock(m_dataLock);
        TelemetryObject telemetryObject = TelemetrySerialiser::deserialise(json);
        getTelemetryObjectByKey(key) = telemetryObject;
    }

    std::string TelemetryHelper::serialiseAndReset()
    {
        std::scoped_lock scopedLock( m_callbackLock, m_dataLock);

        // Serialise
        std::string serialised = TelemetrySerialiser::serialise(m_root);
        locked_reset();
        return serialised;
    }


    void TelemetryHelper::set(const std::string& key, const TelemetryObject& object, bool stick)
    {
        std::lock_guard<std::mutex> lock(m_dataLock);
        TelemetryObject& telemetryObject = getTelemetryObjectByKey(key);
        telemetryObject = object;
        if ( stick)
        {
            TelemetryObject& telemetryObjectStick = getTelemetryObjectByKey(key, m_resetToThis);
            telemetryObjectStick = object;
        }
    }



} // namespace Common::Telemetry