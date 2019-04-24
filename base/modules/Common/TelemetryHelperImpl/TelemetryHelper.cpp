/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetryHelper.h"
#include "TelemetrySerialiser.h"
#include <functional>

void TelemetryHelper::set(const std::string& key, int value)
{
    std::lock_guard<std::mutex> lock(m_lock);
    TelemetryValue telemetryValue(value);
    getTelemetryObjectByKey(key).set(telemetryValue);
}

void TelemetryHelper::set(const std::string& key, unsigned int value)
{
    std::lock_guard<std::mutex> lock(m_lock);
    TelemetryValue telemetryValue(value);
    getTelemetryObjectByKey(key).set(telemetryValue);
}

void TelemetryHelper::set(const std::string& key, const std::string& value)
{
    std::lock_guard<std::mutex> lock(m_lock);
    TelemetryValue telemetryValue(value);
    getTelemetryObjectByKey(key).set(telemetryValue);
}

void TelemetryHelper::set(const std::string& key, const char* value)
{
    std::lock_guard<std::mutex> lock(m_lock);
    TelemetryValue telemetryValue(value);
    getTelemetryObjectByKey(key).set(telemetryValue);
}

void TelemetryHelper::set(const std::string& key, bool value)
{
    std::lock_guard<std::mutex> lock(m_lock);
    TelemetryValue telemetryValue(value);
    getTelemetryObjectByKey(key).set(telemetryValue);
}

void TelemetryHelper::increment(const std::string& key, int value)
{
    std::lock_guard<std::mutex> lock(m_lock);
    TelemetryObject& telemetryObject = getTelemetryObjectByKey(key);
    int newValue = telemetryObject.getValue().getInteger() + value;
    TelemetryValue telemetryValue(newValue);
    telemetryObject.set(telemetryValue);
}

void TelemetryHelper::increment(const std::string& key, unsigned int value)
{
    std::lock_guard<std::mutex> lock(m_lock);
    TelemetryObject& telemetryObject = getTelemetryObjectByKey(key);
    unsigned int newValue = telemetryObject.getValue().getInteger() + value;
    TelemetryValue telemetryValue(newValue);
    telemetryObject.set(telemetryValue);
}

void TelemetryHelper::append(const std::string& key, int value)
{
    std::lock_guard<std::mutex> lock(m_lock);
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

void TelemetryHelper::append(const std::string& key, unsigned int value)
{
    std::lock_guard<std::mutex> lock(m_lock);
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

void TelemetryHelper::append(const std::string& key, const std::string& value)
{
    std::lock_guard<std::mutex> lock(m_lock);
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

void TelemetryHelper::append(const std::string& key, const char* value)
{
    std::lock_guard<std::mutex> lock(m_lock);
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

void TelemetryHelper::append(const std::string& key, bool value)
{
    std::lock_guard<std::mutex> lock(m_lock);
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
    std::lock_guard<std::mutex> lock(m_lock);
    Common::Telemetry::TelemetrySerialiser serialiser;
    return serialiser.serialise(m_root);
}
