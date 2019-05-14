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
    const Telemetry::TelemetryConfig::Config& config)
{
    nlohmann::json j = config;
    return j.dump();
}

void Telemetry::TelemetryConfig::to_json(nlohmann::json& j, const Telemetry::TelemetryConfig::Config& config)
{
   // j = nlohmann::json{{"port", config.m_port}, {"address", config.m_server}};
    j = nlohmann::json {
        {"server", config.m_server},
        {"resourceRoute", config.m_resourceRoute},
        {"port", config.m_port},
        {"headers", config.m_headers},
        {"verb", config.m_verb},
        {"proxies", config.m_proxies},
        {"messageRelays", config.m_messageRelays},
        {"externalProcessTimeout", config.m_externalProcessTimeout},
        {"maxJsonSize", config.m_maxJsonSize}
    };

}

void Telemetry::TelemetryConfig::to_json(nlohmann::json& j, const Telemetry::TelemetryConfig::Proxy& proxy)
{
    j = nlohmann::json
    {
        {"url", proxy.m_url},
        {"port", proxy.m_port},
        {"authentication", proxy.m_authentication},
        {"username", proxy.m_username },
        {"password", proxy.m_password}
    };
}

void Telemetry::TelemetryConfig::to_json(nlohmann::json& j, const Telemetry::TelemetryConfig::MessageRelay& messageRelay)
{
    j = nlohmann::json
    {
        {"certPath", messageRelay.m_certificatePath},
        {"url", messageRelay.m_url},
        {"port", messageRelay.m_port},
        {"authentication", messageRelay.m_authentication},
        {"username", messageRelay.m_username },
        {"password", messageRelay.m_password}
    };
}

Telemetry::TelemetryConfig::Config Telemetry::TelemetryConfig::TelemetryConfigSerialiser::deserialise(
    const std::string& jsonString)
{
    nlohmann::json j =  nlohmann::json::parse(jsonString);
    Telemetry::TelemetryConfig::Config config = j;
    return config;
}

void Telemetry::TelemetryConfig::from_json(const nlohmann::json& j, Config& config)
{
    j.at("server").get_to(config.m_server);
    j.at("resourceRoute").get_to(config.m_resourceRoute);
    j.at("port").get_to(config.m_port);
    j.at("headers").get_to(config.m_headers);
    j.at("verb").get_to(config.m_verb);
    j.at("verb").get_to(config.m_verb);
    j.at("proxies").get_to(config.m_proxies);
    j.at("messageRelays").get_to(config.m_messageRelays);
    j.at("externalProcessTimeout").get_to(config.m_externalProcessTimeout);
    j.at("maxJsonSize").get_to(config.m_maxJsonSize);
}

void Telemetry::TelemetryConfig::from_json(const nlohmann::json& j, Proxy& proxy)
{
    j.at("url").get_to(proxy.m_url);
    j.at("port").get_to(proxy.m_port);
    j.at("authentication").get_to(proxy.m_authentication);
    j.at("username").get_to(proxy.m_username);
    j.at("password").get_to(proxy.m_password);
}

void Telemetry::TelemetryConfig::from_json(const nlohmann::json& j, MessageRelay& messageRelay)
{
    j.at("certPath").get_to(messageRelay.m_certificatePath);
    j.at("url").get_to(messageRelay.m_url);
    j.at("port").get_to(messageRelay.m_port);
    j.at("authentication").get_to(messageRelay.m_authentication);
    j.at("username").get_to(messageRelay.m_username);
    j.at("password").get_to(messageRelay.m_password);
}