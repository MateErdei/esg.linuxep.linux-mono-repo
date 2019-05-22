/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetryConfigSerialiser.h"

#include <Telemetry/LoggerImpl/Logger.h>

#include <iostream>
#include <json.hpp>

namespace Telemetry::TelemetryConfigImpl
{
    void to_json(nlohmann::json& j, const Config& config);
    void from_json(const nlohmann::json& j, Config& config);

    void to_json(nlohmann::json& j, const Proxy& proxy);
    void from_json(const nlohmann::json& j, Proxy& proxy);

    void to_json(nlohmann::json& j, const MessageRelay& messageRelay);
    void from_json(const nlohmann::json& j, MessageRelay& messageRelay);

    void to_json(nlohmann::json& j, const Proxy& proxy)
    {
        j = nlohmann::json{ { "url", proxy.getUrl() },
                            { "port", proxy.getPort() },
                            { "authentication", proxy.getAuthentication() },
                            { "username", proxy.getUsername() },
                            { "password", proxy.getObfuscatedPassword() } };
    }

    void to_json(nlohmann::json& j, const MessageRelay& messageRelay)
    {
        j = nlohmann::json{ { "id", messageRelay.getId() },
                            { "priority", messageRelay.getPriority() },
                            { "url", messageRelay.getUrl() },
                            { "port", messageRelay.getPort() },
                            { "authentication", messageRelay.getAuthentication() },
                            { "username", messageRelay.getUsername() },
                            { "password", messageRelay.getObfuscatedPassword() } };
    }

    void to_json(nlohmann::json& j, const Config& config)
    {
        j = nlohmann::json{ { "server", config.getServer() },
                            { "resourceRoot", config.getResourceRoot() },
                            { "port", config.getPort() },
                            { "headers", config.getHeaders() },
                            { "verb", config.getVerb() },
                            { "proxies", config.getProxies() },
                            { "messageRelays", config.getMessageRelays() },
                            { "externalProcessWaitTime", config.getExternalProcessWaitTime() },
                            { "externalProcessWaitRetries", config.getExternalProcessWaitRetries() },
                            { "maxJsonSize", config.getMaxJsonSize() },
                            { "telemetryServerCertificatePath", config.getTelemetryServerCertificatePath() } };
    }

    void from_json(const nlohmann::json& j, Proxy& proxy)
    {
        proxy.setUrl(j.at("url"));
        proxy.setPort(j.at("port"));
        proxy.setAuthentication(j.at("authentication"));
        proxy.setUsername(j.at("username"));
        proxy.setPassword(j.at("password"));
    }

    void from_json(const nlohmann::json& j, MessageRelay& messageRelay)
    {
        messageRelay.setId(j.at("id"));
        messageRelay.setPriority(j.at("priority"));
        messageRelay.setUrl(j.at("url"));
        messageRelay.setPort(j.at("port"));
        messageRelay.setAuthentication(j.at("authentication"));
        messageRelay.setUsername(j.at("username"));
        messageRelay.setPassword(j.at("password"));
    }

    void from_json(const nlohmann::json& j, Config& config)
    {
        if (j.contains("server")) config.setServer(j.at("server"));
        if (j.contains("resourceRoot")) config.setResourceRoot(j.at("resourceRoot"));
        if (j.contains("port")) config.setPort(j.at("port"));
        if (j.contains("headers")) config.setHeaders(j.at("headers"));
        if (j.contains("verb")) config.setVerb(j.at("verb"));
        if (j.contains("proxies")) config.setProxies(j.at("proxies"));
        if (j.contains("messageRelays")) config.setMessageRelays(j.at("messageRelays"));
        if (j.contains("externalProcessWaitTime")) config.setExternalProcessWaitTime(j.at("externalProcessWaitTime"));
        if (j.contains("externalProcessWaitRetries")) config.setExternalProcessWaitRetries(j.at("externalProcessWaitRetries"));
        if (j.contains("maxJsonSize")) config.setMaxJsonSize(j.at("maxJsonSize"));
        if (j.contains("telemetryServerCertificatePath")) config.setTelemetryServerCertificatePath(j.at("telemetryServerCertificatePath"));
    }

    std::string TelemetryConfigSerialiser::serialise(const Config& config)
    {
        if (!config.isValid())
        {
            throw std::invalid_argument("Configuration input is invalid and cannot be serialised");
        }

        nlohmann::json j = config;

        return j.dump();
    }

    Config TelemetryConfigSerialiser::deserialise(const std::string& jsonString)
    {
        Config config;

        try
        {
            nlohmann::json j = nlohmann::json::parse(jsonString);
            config = j;
        }
        // As well as basic JSON parsing errors, building config object can also fail, so catch all JSON exceptions.
        catch (nlohmann::detail::exception& e)
        {
            std::stringstream msg;
            msg << "Configuration JSON is invalid: " << e.what();
            LOGERROR(msg.str());
            throw std::runtime_error(msg.str());
        }

        if (!config.isValid())
        {
            throw std::runtime_error("Configuration from deserialised JSON is invalid");
        }

        return config;
    }
}