/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetryObject.h"

#include <sstream>

Common::Telemetry::TelemetryObject::TelemetryObject() :
    m_type(Type::object),
    m_value(std::map<std::string, TelemetryObject>())
{
}

void Common::Telemetry::TelemetryObject::set(const std::string& key, const Common::Telemetry::TelemetryValue& value)
{
    if (!std::holds_alternative<std::map<std::string, TelemetryObject>>(m_value))
    {
        m_value = std::map<std::string, TelemetryObject>();
    }

    auto& nodes = std::get<std::map<std::string, TelemetryObject>>(m_value);
    TelemetryObject obj;
    obj.set(value);
    nodes[key] = obj;
    m_type = Type::object;
}

void Common::Telemetry::TelemetryObject::set(const std::string& key, const Common::Telemetry::TelemetryObject& value)
{
    if (!std::holds_alternative<std::map<std::string, TelemetryObject>>(m_value))
    {
        m_value = std::map<std::string, TelemetryObject>();
    }

    auto& nodes = std::get<std::map<std::string, TelemetryObject>>(m_value);
    nodes[key] = value;
}

void Common::Telemetry::TelemetryObject::set(
    const std::string& key,
    const std::list<Common::Telemetry::TelemetryObject>& objectList)
{
    if (!std::holds_alternative<std::map<std::string, TelemetryObject>>(m_value))
    {
        m_value = std::map<std::string, TelemetryObject>();
    }

    auto& nodes = std::get<std::map<std::string, TelemetryObject>>(m_value);
    TelemetryObject obj;
    obj.set(objectList);
    nodes[key] = obj;
    m_type = Type::object;
}

void Common::Telemetry::TelemetryObject::set(const Common::Telemetry::TelemetryValue& value)
{
    m_value = value;
    m_type = Type::value;
}

void Common::Telemetry::TelemetryObject::set(const std::list<Common::Telemetry::TelemetryObject>& value)
{
    m_value = value;
    m_type = Type::array;
}

Common::Telemetry::TelemetryObject& Common::Telemetry::TelemetryObject::getObject(const std::string& key)
{
    checkType(Type::object);
    auto& nodes = std::get<std::map<std::string, TelemetryObject>>(m_value);
    return nodes.at(key);
}

Common::Telemetry::TelemetryValue& Common::Telemetry::TelemetryObject::getValue()
{
    checkType(Type::value);
    auto& value = std::get<TelemetryValue>(m_value);
    return value;
}

const Common::Telemetry::TelemetryValue& Common::Telemetry::TelemetryObject::getValue() const
{
    checkType(Type::value);
    const auto& value = std::get<TelemetryValue>(m_value);
    return value;
}

std::list<Common::Telemetry::TelemetryObject>& Common::Telemetry::TelemetryObject::getArray()
{
    checkType(Type::array);
    auto& list = std::get<std::list<TelemetryObject>>(m_value);
    return list;
}

const std::list<Common::Telemetry::TelemetryObject>& Common::Telemetry::TelemetryObject::getArray() const
{
    checkType(Type::array);
    const auto& list = std::get<std::list<TelemetryObject>>(m_value);
    return list;
}

std::map<std::string, Common::Telemetry::TelemetryObject>& Common::Telemetry::TelemetryObject::getChildObjects()
{
    checkType(Type::object);
    auto& obj = std::get<std::map<std::string, TelemetryObject>>(m_value);
    return obj;
}

const std::map<std::string, Common::Telemetry::TelemetryObject>& Common::Telemetry::TelemetryObject::getChildObjects()
    const
{
    checkType(Type::object);
    const auto& obj = std::get<std::map<std::string, TelemetryObject>>(m_value);
    return obj;
}

Common::Telemetry::TelemetryObject::Type Common::Telemetry::TelemetryObject::getType() const
{
    return m_type;
}

bool Common::Telemetry::TelemetryObject::keyExists(const std::string& key)
{
    checkType(Type::object);
    auto& nodes = std::get<std::map<std::string, TelemetryObject>>(m_value);
    return nodes.find(key) != nodes.end();
}

bool Common::Telemetry::TelemetryObject::operator==(const Common::Telemetry::TelemetryObject& rhs) const
{
    return m_type == rhs.m_type && m_value == rhs.m_value;
}

bool Common::Telemetry::TelemetryObject::operator!=(const Common::Telemetry::TelemetryObject& rhs) const
{
    return !(rhs == *this);
}

void Common::Telemetry::TelemetryObject::checkType(Type expectedType) const
{
    if (m_type != expectedType)
    {
        std::stringstream msg;
        msg << "Telemetry object does not contain the expected type. Expected: " << static_cast<int>(expectedType)
            << "Actual: " << static_cast<int>(m_type);
        throw std::logic_error(msg.str());
    }
}
