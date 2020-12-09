/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetryJsonToMap.h"

#include <json.hpp>
#include <sstream>

namespace Common::Telemetry
{
    std::unordered_map<std::string, TelemetryValue> flatJsonToMap(const std::string& jsonString)
    {
        std::unordered_map<std::string, TelemetryValue> map;
        try
        {
            nlohmann::json jsonObj = nlohmann::json::parse(jsonString);
            for (auto& item : jsonObj.items())
            {
                // Only interpret the top level of the json string passed ignore arrays and objects
                if (item.value().is_primitive() && !item.value().is_null())
                {
                    if (item.value().is_string())
                    {
                        map[item.key()] = TelemetryValue(item.value().get<std::string>());
                    }
                    else if (item.value().is_number_unsigned())
                    {
                        map[item.key()] = TelemetryValue(item.value().get<unsigned long>());
                    }
                    else if (item.value().is_number_integer())
                    {
                        map[item.key()] = TelemetryValue(item.value().get<long>());
                    }
                    else if (item.value().is_number_float())
                    {
                        map[item.key()] = TelemetryValue(item.value().get<double>());
                    }
                    else if (item.value().is_boolean())
                    {
                        map[item.key()] = TelemetryValue(item.value().get<bool>());
                    }
                }
            }
        }
        catch (const nlohmann::detail::exception& e)
        {
            std::stringstream msg;
            msg << "JSON conversion to map failed, JSON is invalid: " << e.what();
            throw std::runtime_error(msg.str());
        }
        return map;
    }
} // namespace Common::Telemetry