/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetryConfigSerialiser.h"

#include <iostream>
#include <json.hpp>

using namespace Telemetry::TelemetryConfig;

std::string TelemetryConfigSerialiser::serialise(const Config& config)
{
    if (!config.isValid())
    {
        throw std::invalid_argument("Configuration input is invalid and cannot be serialised");
    }

    nlohmann::json j = config;

    return j.dump();
}

void Telemetry::TelemetryConfig::to_json(nlohmann::json& j, const Config& config)
{
    j = nlohmann::json{ { "server", config.getServer() },
                        { "resourceRoute", config.getResourceRoute() },
                        { "port", config.getPort() },
                        { "headers", config.getHeaders() },
                        { "verb", config.getVerb() },
                        { "proxies", config.getProxies() },
                        { "messageRelays", config.getMessageRelays() },
                        { "externalProcessTimeout", config.getExternalProcessTimeout() },
                        { "externalProcessRetries", config.getExternalProcessRetries() },
                        { "maxJsonSize", config.getMaxJsonSize() },
                        { "telemetryServerCertificatePath", config.getTelemetryServerCertificatePath() } };
}

void Telemetry::TelemetryConfig::to_json(nlohmann::json& j, const Proxy& proxy)
{
    j = nlohmann::json{ { "url", proxy.getUrl() },
                        { "port", proxy.getPort() },
                        { "authentication", proxy.getAuthentication() },
                        { "username", proxy.getUsername() },
                        { "password", proxy.getObfuscatedPassword() } };
}

void Telemetry::TelemetryConfig::to_json(nlohmann::json& j, const MessageRelay& messageRelay)
{
    j = nlohmann::json{ { "id", messageRelay.getId() },
                        { "priority", messageRelay.getPriority() },
                        { "url", messageRelay.getUrl() },
                        { "port", messageRelay.getPort() },
                        { "authentication", messageRelay.getAuthentication() },
                        { "username", messageRelay.getUsername() },
                        { "password", messageRelay.getObfuscatedPassword() } };
}

Config TelemetryConfigSerialiser::deserialise(const std::string& jsonString)
{
    nlohmann::json j = nlohmann::json::parse(jsonString);
    Config config = j;

    if (!config.isValid())
    {
        throw std::invalid_argument("Configuration output from deserialised JSON is invalid");
    }

    return config;
}

void Telemetry::TelemetryConfig::from_json(const nlohmann::json& j, Config& config)
{
    config.setServer(j.at("server"));
    config.setResourceRoute(j.at("resourceRoute"));
    config.setPort(j.at("port"));
    config.setHeaders(j.at("headers"));
    config.setVerb(j.at("verb"));
    config.setProxies(j.at("proxies"));
    config.setMessageRelays(j.at("messageRelays"));
    config.setExternalProcessTimeout(j.at("externalProcessTimeout"));
    config.setExternalProcessRetries(j.at("externalProcessRetries"));
    config.setMaxJsonSize(j.at("maxJsonSize"));
    config.setTelemetryServerCertificatePath(j.at("telemetryServerCertificatePath"));
}

void Telemetry::TelemetryConfig::from_json(const nlohmann::json& j, Proxy& proxy)
{
    proxy.setUrl(j.at("url"));
    proxy.setPort(j.at("port"));
    proxy.setAuthentication(j.at("authentication"));
    proxy.setUsername(j.at("username"));
    proxy.setPassword(j.at("password"));
}

void Telemetry::TelemetryConfig::from_json(const nlohmann::json& j, MessageRelay& messageRelay)
{
    messageRelay.setId(j.at("id"));
    messageRelay.setPriority(j.at("priority"));
    messageRelay.setUrl(j.at("url"));
    messageRelay.setPort(j.at("port"));
    messageRelay.setAuthentication(j.at("authentication"));
    messageRelay.setUsername(j.at("username"));
    messageRelay.setPassword(j.at("password"));
}