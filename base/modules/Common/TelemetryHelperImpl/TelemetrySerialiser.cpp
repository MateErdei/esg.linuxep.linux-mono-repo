/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetrySerialiser.h"

#include <sstream>
#include <stdexcept>

namespace Common::Telemetry
{
    void to_json(nlohmann::json& j, const TelemetryValue& value)
    {
        auto type = value.getType();
        switch (type)
        {
            case TelemetryValue::Type::integer_type:
            {
                j = value.getInteger();
                break;
            }

            case TelemetryValue::Type::unsigned_integer_type:
            {
                j = value.getUnsignedInteger();
                break;
            }

            case TelemetryValue::Type::double_type:
            {
                j = value.getDouble();
                break;
            }

            case TelemetryValue::Type::string_type:
            {
                j = value.getString();
                break;
            }

            case TelemetryValue::Type::boolean_type:
            {
                j = value.getBoolean();
                break;
            }

            default:
            {
                std::stringstream msg;
                msg << "Cannot serialise telemetry value. Invalid type: " << static_cast<int>(type);
                throw std::invalid_argument(msg.str());
            }
        }
    }

    void from_json(const nlohmann::json& j, TelemetryValue& value)
    {
        auto type = j.type();
        switch (type)
        {
            case nlohmann::detail::value_t::number_integer:
            {
                value.set(j.get<long>());
                break;
            }

            case nlohmann::detail::value_t::number_unsigned:
            {
                value.set(j.get<unsigned long>());
                break;
            }

            case nlohmann::detail::value_t::number_float:
            {
                value.set(j.get<double>());
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
                std::stringstream msg;
                msg << "Cannot deserialise value. Invalid type: " << static_cast<int>(type);
                throw std::invalid_argument(msg.str());
            }
        }
    }

    void to_json(nlohmann::json& j, const TelemetryObject& telemetryObject)
    {
        auto type = telemetryObject.getType();
        switch (type)
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
                std::stringstream msg;
                msg << "Cannot serialise telemetry object. Invalid type: " << static_cast<int>(type);
                throw std::invalid_argument(msg.str());
            }
        }
    }

    void from_json(const nlohmann::json& j, TelemetryObject& telemetryObject)
    {
        for (const auto& item : j.items())
        {
            auto type = item.value().type();
            switch (type)
            {
                case nlohmann::detail::value_t::number_unsigned:
                case nlohmann::detail::value_t::number_integer:
                case nlohmann::detail::value_t::number_float:
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
                    std::stringstream msg;
                    msg << "Cannot deserialise json item. Invalid type: " << static_cast<int>(type);
                    throw std::invalid_argument(msg.str());
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