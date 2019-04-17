/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetrySerialiser.h"
#include <exception>

namespace Common::Telemetry
{

    std::string TelemetrySerialiser::serialise(const Common::Telemetry::TelemetryNode& /*node*/)
    {
        return "";
    }

    void to_json(nlohmann::json& j, const TelemetryValue& value)
    {
        auto type = value.getValueType();
        switch(type)
        {
            case ValueType::integer_type:
            {
                j[value.getKey()] = value.getInteger();
                break;
            }

            case ValueType::string_type:
            {
                j[value.getKey()] = value.getString();
                break;
            }

            case ValueType::boolean_type:
            {
                j[value.getKey()] = value.getBoolean();
                break;
            }

            default:
            {
                throw std::invalid_argument("Cannot serialise value. Invalid type");
            }
        }
    }

    void to_json(nlohmann::json& j, const TelemetryDictionary& dict)
    {
        auto object = dict.getDictionary();
        for(const auto& node: object)
        {
            auto type = node->getType();
            switch(type)
            {
                case NodeType::dict:
                {
                    auto nestedDict = std::static_pointer_cast<TelemetryDictionary>(node);
                    j[nestedDict->getKey()] = *nestedDict;
                    break;
                }

                case NodeType::value:
                {
                    auto nestedValue = std::static_pointer_cast<TelemetryValue>(node);
                    to_json(j, *nestedValue);
                    break;
                }

                default:
                {
                    throw std::invalid_argument("Cannot serialise node. Invalid type");
                }
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
                throw std::invalid_argument("Cannot serialise value. Invalid type");
            }
        }
    }

    void from_json(const nlohmann::json& j, TelemetryDictionary& dict)
    {
        for(const auto& item: j.items())
        {
            switch(item.value().type())
            {
                case nlohmann::detail::value_t::number_integer:
                case nlohmann::detail::value_t::string:
                case nlohmann::detail::value_t::boolean:
                {
                    auto nestedValue = std::make_shared<TelemetryValue>(item.key());
                    dict.set(nestedValue);
                    from_json(item.value(), *nestedValue);
                    break;
                }

                case nlohmann::detail::value_t::object:
                {
                    auto nestedDict = std::make_shared<TelemetryDictionary>(item.key());
                    dict.set(nestedDict);
                    from_json(item.value(), *nestedDict);
                    break;
                }

                default:
                {
                    throw std::invalid_argument("Cannot deserialise value. Invalid type");
                }
            }
        }
    }
}