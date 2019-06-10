/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Serialiser.h"

#include <Telemetry/LoggerImpl/Logger.h>

#include <iostream>
#include <json.hpp>

namespace Common::TelemetryConfigImpl
{
    const std::string URL_PROXY_KEY = "url";
    const std::string PORT_PROXY_KEY = "port";
    const std::string AUTH_PROXY_KEY = "authentication";
    const std::string USERNAME_PROXY_KEY = "username";
    const std::string PASSWORD_PROXY_KEY = "password";

    const std::string ID_MR_KEY = "id";
    const std::string PRIORITY_MR_KEY = "priority";
    const std::string URL_MR_KEY = "url";
    const std::string PORT_MR_KEY = "port";
    const std::string AUTH_MR_KEY = "authentication";
    const std::string USERNAME_MR_KEY = "username";
    const std::string PASSWORD_MR_KEY = "password";

    const std::string SERVER_CONFIG_KEY = "server";
    const std::string RESOURCE_CONFIG_KEY = "resourceRoot";
    const std::string PORT_CONFIG_KEY = "port";
    const std::string HEADERS_CONFIG_KEY = "additionalHeaders";
    const std::string VERB_CONFIG_KEY = "verb";
    const std::string PROXIES_CONFIG_KEY = "proxies";
    const std::string MR_CONFIG_KEY = "messageRelays";
    const std::string EPWT_CONFIG_KEY = "externalProcessWaitTime";
    const std::string EPWR_CONFIG_KEY = "externalProcessWaitRetries";
    const std::string MAX_JSON_CONFIG_KEY = "maxJsonSize";
    const std::string CERT_PATH_CONFIG_KEY = "telemetryServerCertificatePath";

    void to_json(nlohmann::json& j, const Config& config);
    void from_json(const nlohmann::json& j, Config& config);

    void to_json(nlohmann::json& j, const Proxy& proxy);
    void from_json(const nlohmann::json& j, Proxy& proxy);

    void to_json(nlohmann::json& j, const MessageRelay& messageRelay);
    void from_json(const nlohmann::json& j, MessageRelay& messageRelay);

    void to_json(nlohmann::json& j, const Proxy& proxy)
    {
        j = nlohmann::json{ { URL_PROXY_KEY, proxy.getUrl() },
                            { PORT_PROXY_KEY, proxy.getPort() },
                            { AUTH_PROXY_KEY, proxy.getAuthentication() },
                            { USERNAME_PROXY_KEY, proxy.getUsername() },
                            { PASSWORD_PROXY_KEY, proxy.getObfuscatedPassword() } };
    }

    void to_json(nlohmann::json& j, const MessageRelay& messageRelay)
    {
        j = nlohmann::json{ { ID_MR_KEY, messageRelay.getId() },
                            { PRIORITY_MR_KEY, messageRelay.getPriority() },
                            { URL_MR_KEY, messageRelay.getUrl() },
                            { PORT_MR_KEY, messageRelay.getPort() },
                            { AUTH_MR_KEY, messageRelay.getAuthentication() },
                            { USERNAME_PROXY_KEY, messageRelay.getUsername() },
                            { PASSWORD_MR_KEY, messageRelay.getObfuscatedPassword() } };
    }

    void to_json(nlohmann::json& j, const Config& config)
    {
        j = nlohmann::json{ { SERVER_CONFIG_KEY, config.getServer() },
                            { RESOURCE_CONFIG_KEY, config.getResourceRoot() },
                            { PORT_CONFIG_KEY, config.getPort() },
                            { HEADERS_CONFIG_KEY, config.getHeaders() },
                            { VERB_CONFIG_KEY, config.getVerb() },
                            { PROXIES_CONFIG_KEY, config.getProxies() },
                            { MR_CONFIG_KEY, config.getMessageRelays() },
                            { EPWT_CONFIG_KEY, config.getExternalProcessWaitTime() },
                            { EPWR_CONFIG_KEY, config.getExternalProcessWaitRetries() },
                            { MAX_JSON_CONFIG_KEY, config.getMaxJsonSize() },
                            { CERT_PATH_CONFIG_KEY, config.getTelemetryServerCertificatePath() } };
    }

    void from_json(const nlohmann::json& j, Proxy& proxy)
    {
        proxy.setUrl(j.at(URL_PROXY_KEY));
        proxy.setPort(j.at(PORT_PROXY_KEY));
        proxy.setAuthentication(j.at(AUTH_PROXY_KEY));
        proxy.setUsername(j.at(USERNAME_PROXY_KEY));
        proxy.setPassword(j.at(PASSWORD_PROXY_KEY));
    }

    void from_json(const nlohmann::json& j, MessageRelay& messageRelay)
    {
        messageRelay.setId(j.at(ID_MR_KEY));
        messageRelay.setPriority(j.at(PRIORITY_MR_KEY));
        messageRelay.setUrl(j.at(URL_MR_KEY));
        messageRelay.setPort(j.at(PORT_MR_KEY));
        messageRelay.setAuthentication(j.at(AUTH_MR_KEY));
        messageRelay.setUsername(j.at(USERNAME_PROXY_KEY));
        messageRelay.setPassword(j.at(PASSWORD_MR_KEY));
    }

    void from_json(const nlohmann::json& j, Config& config)
    {
        if (j.contains(SERVER_CONFIG_KEY))
        {
            config.setServer(j.at(SERVER_CONFIG_KEY));
        }

        if (j.contains(RESOURCE_CONFIG_KEY))
        {
            config.setResourceRoot(j.at(RESOURCE_CONFIG_KEY));
        }

        if (j.contains(PORT_CONFIG_KEY))
        {
            config.setPort(j.at(PORT_CONFIG_KEY));
        }

        if (j.contains(HEADERS_CONFIG_KEY))
        {
            config.setHeaders(j.at(HEADERS_CONFIG_KEY));
        }

        if (j.contains(VERB_CONFIG_KEY))
        {
            config.setVerb(j.at(VERB_CONFIG_KEY));
        }

        if (j.contains(PROXIES_CONFIG_KEY))
        {
            config.setProxies(j.at(PROXIES_CONFIG_KEY));
        }

        if (j.contains(MR_CONFIG_KEY))
        {
            config.setMessageRelays(j.at(MR_CONFIG_KEY));
        }

        if (j.contains(EPWT_CONFIG_KEY))
        {
            config.setExternalProcessWaitTime(j.at(EPWT_CONFIG_KEY));
        }

        if (j.contains(EPWR_CONFIG_KEY))
        {
            config.setExternalProcessWaitRetries(j.at(EPWR_CONFIG_KEY));
        }

        if (j.contains(MAX_JSON_CONFIG_KEY))
        {
            config.setMaxJsonSize(j.at(MAX_JSON_CONFIG_KEY));
        }

        if (j.contains(CERT_PATH_CONFIG_KEY))
        {
            config.setTelemetryServerCertificatePath(j.at(CERT_PATH_CONFIG_KEY));
        }
    }

    std::string Serialiser::serialise(const Config& config)
    {
        if (!config.isValid())
        {
            throw std::invalid_argument("Configuration input is invalid and cannot be serialised");
        }

        nlohmann::json j = config;

        return j.dump();
    }

    Config Serialiser::deserialise(const std::string& jsonString)
    {
        Config config;

        try
        {
            nlohmann::json j = nlohmann::json::parse(jsonString);
            config = j;
        }
        // As well as basic JSON parsing errors, building config object can also fail, so catch all JSON exceptions.
        catch (const nlohmann::detail::exception& e)
        {
            std::stringstream msg;
            msg << "Configuration JSON is invalid: " << e.what();
            throw std::runtime_error(msg.str());
        }

        if (!config.isValid())
        {
            throw std::runtime_error("Configuration from deserialised JSON is invalid");
        }

        return config;
    }
}