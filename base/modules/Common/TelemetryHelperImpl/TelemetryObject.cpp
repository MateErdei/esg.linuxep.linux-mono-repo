//
// Created by pair on 17/04/19.
//

#include "TelemetryObject.h"

Common::Telemetry::TelemetryObject::TelemetryObject()
    : m_type(Type::object)
{

}

//void Common::Telemetry::TelemetryObject::set(std::string key, const char* value)
//{
//    set(key, std::string(value));
//}
//
//void Common::Telemetry::TelemetryObject::set(std::string key, std::string value)
//{
//    TelemetryObject obj;
//    obj.set(value);
//    m_nodes[key] = obj;
//}
//
//void Common::Telemetry::TelemetryObject::set(std::string key, bool value)
//{
//    TelemetryObject obj;
//    obj.set(value);
//    m_nodes[key] = obj;
//}
//
//void Common::Telemetry::TelemetryObject::set(std::string key, int value)
//{
//    TelemetryObject obj;
//    obj.set(value);
//    m_nodes[key] = obj;
//}

void Common::Telemetry::TelemetryObject::set(std::string key, Common::Telemetry::TelemetryObject value)
{
    m_nodes[key] = value;
}

//void Common::Telemetry::TelemetryObject::set(const char* value)
//{
//    set(std::string(value));
//}

//void Common::Telemetry::TelemetryObject::set(std::string value)
//{
//    m_value.set(value);
//    m_type = Type::value;
//}
//
//void Common::Telemetry::TelemetryObject::set(int value)
//{
//    m_value.set(value);
//    m_type = Type::value;
//}
//
//void Common::Telemetry::TelemetryObject::set(bool value)
//{
//    m_value.set(value);
//    m_type = Type::value;
//}

void Common::Telemetry::TelemetryObject::set(std::list<Common::Telemetry::TelemetryObject> value)
{
    m_array = value;
    m_type = Type::array;
}

Common::Telemetry::TelemetryObject::Type Common::Telemetry::TelemetryObject::getType() const
{
    return m_type;
}

//std::string Common::Telemetry::TelemetryObject::getString()
//{
//    checkType(Type::value);
//    return m_value.getString();
//}
//
//int Common::Telemetry::TelemetryObject::getInt()
//{
//    checkType(Type::value);
//    return m_value.getInteger();
//}
//
//bool Common::Telemetry::TelemetryObject::getBool()
//{
//    checkType(Type::value);
//    return m_value.getBoolean();
//}

std::list<Common::Telemetry::TelemetryObject> Common::Telemetry::TelemetryObject::getArray() const
{
    checkType(Type::array);
    return m_array;
}

Common::Telemetry::TelemetryObject Common::Telemetry::TelemetryObject::getObject(std::string key)
{
    checkType(Type::object);
    return m_nodes.at(key);
}

bool Common::Telemetry::TelemetryObject::keyExists(std::string key)
{
    return m_nodes.find(key) != m_nodes.end();
}

void Common::Telemetry::TelemetryObject::checkType(Type expectedType) const
{
    if(m_type != expectedType)
    {
        throw std::invalid_argument("bad type");
    }
}

const std::map<std::string, Common::Telemetry::TelemetryObject>& Common::Telemetry::TelemetryObject::getChildObjects() const
{
    return m_nodes;
}

//Common::Telemetry::TelemetryValue::ValueType Common::Telemetry::TelemetryObject::getValueType()
//{
//    return m_value.getValueType();
//}

Common::Telemetry::TelemetryValue Common::Telemetry::TelemetryObject::getValue() const
{
    return m_value;
}

void Common::Telemetry::TelemetryObject::set(Common::Telemetry::TelemetryValue value)
{
    m_value = value;
    m_type = Type::value;
}

void Common::Telemetry::TelemetryObject::set(std::string key, Common::Telemetry::TelemetryValue value)
{
    TelemetryObject obj;
    obj.set(value);
    m_nodes[key] = obj;
    m_type = Type::object;
}

void Common::Telemetry::TelemetryObject::set(std::string key, std::list<Common::Telemetry::TelemetryObject> objectList)
{
    TelemetryObject obj;
    obj.set(objectList);
    m_nodes[key] = obj;
    m_type = Type::object;

}

bool Common::Telemetry::TelemetryObject::operator==(const Common::Telemetry::TelemetryObject& rhs) const
{
    return m_type == rhs.m_type &&
           m_nodes == rhs.m_nodes &&
           m_array == rhs.m_array &&
           m_value == rhs.m_value;
}

bool Common::Telemetry::TelemetryObject::operator!=(const Common::Telemetry::TelemetryObject& rhs) const
{
    return !(rhs == *this);
}
