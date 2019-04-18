/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetrySerialiser.h"
#include <exception>

namespace Common::Telemetry
{

//    std::string TelemetrySerialiser::serialise(const Common::Telemetry::TelemetryNode& /*node*/)
//    {
//        return "";
//    }

    void to_json(nlohmann::json& j, const TelemetryValue& value)
    {
        auto type = value.getValueType();
        switch(type)
        {
            case TelemetryValue::ValueType::integer_type:
            {
                j = value.getInteger();
                break;
            }

            case TelemetryValue::ValueType::string_type:
            {
                j = value.getString();
                break;
            }

            case TelemetryValue::ValueType::boolean_type:
            {
                j = value.getBoolean();
                break;
            }

            default:
            {
                throw std::invalid_argument("Cannot to_json(nlohmann::json& j, const TelemetryValue& value) serialise value. Invalid type");
            }
        }
    }

    void from_json(const nlohmann::json& j, TelemetryValue& value)
    {
        switch(j.type())
        {
            case nlohmann::detail::value_t::number_integer:
            {
                value.set(j.get<int>());
                break;
            }

            case nlohmann::detail::value_t::string:
            {
                value.set(j.get<std::string>());
                break;
            }

            case nlohmann::detail::value_t::boolean:
            {
                value.set(j.get<bool>());
                break;
            }

            default:
            {
                throw std::invalid_argument("Cannot serialise value. Invalid type: " + j.dump());
            }
        }
    }

    void to_json(nlohmann::json& j, const TelemetryObject& telemetryObject)
    {
        auto type = telemetryObject.getType();
        switch(type)
        {
            case TelemetryObject::Type::object:
            {
                j = telemetryObject.getChildObjects();
                break;
            }

            case TelemetryObject::Type::array:
            {
                j = telemetryObject.getArray();
                break;
            }

            case TelemetryObject::Type::value:
            {
                to_json(j, telemetryObject.getValue());
                break;
            }

            default:
            {
                throw std::invalid_argument("Cannot to_json(nlohmann::json& j, const TelemetryObject& telemetryObject) serialise node. Invalid type");
            }
        }
    }

    void from_json(const nlohmann::json& j, TelemetryObject& telemetryObject)
    {
        for(const auto& item: j.items())
        {
            switch(item.value().type())
            {
                case nlohmann::detail::value_t::number_integer:
                case nlohmann::detail::value_t::string:
                case nlohmann::detail::value_t::boolean:
                {
                    if (item.key().empty())
                    {
                        telemetryObject.set(item.value().get<TelemetryValue>());
                    }
                    else
                    {
                        telemetryObject.set(item.key(), item.value().get<TelemetryValue>());
                    }
                    break;
                }

                case nlohmann::detail::value_t::object:
                {
                    telemetryObject.set(item.key(), item.value().get<TelemetryObject>());
                    break;
                }

                case nlohmann::detail::value_t::array:
                {
                    telemetryObject.set(item.key(), item.value().get<std::list<TelemetryObject>>());
                    break;
                }

                default:
                {
                    throw std::invalid_argument("Cannot deserialise value. Invalid type");
                }
            }
        }
    }

    std::string TelemetrySerialiser::serialise(const Common::Telemetry::TelemetryObject& telemetryObject)
    {
        nlohmann::json j = telemetryObject;
        return j.dump();
    }

    Common::Telemetry::TelemetryObject TelemetrySerialiser::deserialise(const std::string& jsonString)
    {
        nlohmann::json j = nlohmann::json::parse(jsonString);
        return j.get<TelemetryObject>();
    }
}