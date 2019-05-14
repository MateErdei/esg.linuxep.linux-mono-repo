/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetryConfigSerialiser.h"

#include <iostream>
#include <json.hpp>

//
//using namespace Telemetry::Config;
//
//std::string Telemetry::Config::TelemetryConfigSerialiser::serialise(const Config& telemetryConfigObject)
//{
////    nlohmann::json j = telemetryConfigObject;
////    return j.dump();
//    return "";
//}
//Telemetry::Config::Config Telemetry::Config::TelemetryConfigSerialiser::deserialise(const std::string& jsonString)
//{
////    nlohmann::json j = nlohmann::json::parse(jsonString);
////    return j.get<TelemetryConfig>();
//
//}
//
//void Telemetry::to_json(nlohmann::json& j, const Config& node)
//{
///*
// *  auto type = value.getType();
//        switch (type)
//        {
//            case TelemetryValue::Type::integer_type:
//            {
//                j = value.getInteger();
//                break;
//            }
//
//            case TelemetryValue::Type::unsigned_integer_type:
//            {
//                j = value.getUnsignedInteger();
//                break;
//            }
//
//            case TelemetryValue::Type::string_type:
//            {
//                j = value.getString();
//                break;
//            }
//
//            case TelemetryValue::Type::boolean_type:
//            {
//                j = value.getBoolean();
//                break;
//            }
//
//            default:
//            {
//                std::stringstream msg;
//                msg << "Cannot serialise telemetry value. Invalid type: " << static_cast<int>(type);
//                throw std::invalid_argument(msg.str());
//            }
//        }
// */
//}
//
//std::string Telemetry::Config::TelemetryConfigSerialiser::serialise(
//    const Telemetry::Config::Config& telemetryConfigObject)
//{
//    return std::__cxx11::string();
//}

std::string Telemetry::TelemetryConfig::TelemetryConfigSerialiser::serialise(
    const Telemetry::TelemetryConfig::Config& telemetryConfigObject)
{
    ////    nlohmann::json j = telemetryConfigObject;
    ////    return j.dump();
    std::cout << telemetryConfigObject.m_port;
    return "";
}

void Telemetry::TelemetryConfig::to_json(nlohmann::json& j, const Telemetry::TelemetryConfig::Config& config)
{
    j = nlohmann::json{{"port", config.m_port}, {"address", config.m_server}};

    // TODO this....


}
