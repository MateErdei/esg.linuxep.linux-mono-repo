/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Constants.h"
#include "MessageRelay.h"
#include "Proxy.h"

#include <string>
#include <vector>

namespace Common::TelemetryConfigImpl
{
    /**
     * This configuration is using for external configuration of SSPL telemetry via a warehouse file,
     * as well as internal configuration of the Telemetry Executable by the Telemetry Scheduler via the former's
     * configuration file. Consequently, some settings are used by both and some by only one of these configuration
     * files.
     */
    class Config
    {
    public:
        Config();

        static Config buildExeConfigFromTelemetryConfig(
            const Config& supplementaryConfig,
            const std::string& resourceName);

        const std::string& getServer() const;
        void setServer(const std::string& server);

        const std::string& getResourceRoot() const;
        void setResourceRoot(const std::string& resourceRoot);

        const std::string& getResourcePath() const;
        void setResourcePath(const std::string& resourceRoot);

        unsigned int getPort() const;
        void setPort(unsigned int port);

        const std::vector<std::string>& getHeaders() const;
        void setHeaders(const std::vector<std::string>& headers);

        std::string getVerb() const;
        void setVerb(const std::string& verb);

        unsigned int getInterval() const;
        void setInterval(unsigned int externalProcessWaitTime);

        const std::vector<Proxy>& getProxies() const;
        void setProxies(const std::vector<Proxy>& proxies);

        const std::vector<MessageRelay>& getMessageRelays() const;
        void setMessageRelays(const std::vector<MessageRelay>& messageRelays);

        unsigned int getExternalProcessWaitTime() const;
        void setExternalProcessWaitTime(unsigned int externalProcessWaitTime);

        unsigned int getExternalProcessWaitRetries() const;
        void setExternalProcessWaitRetries(unsigned int externalProcessWaitRetries);

        unsigned int getMaxJsonSize() const;
        void setMaxJsonSize(unsigned int maxJsonSize);

        const std::string& getTelemetryServerCertificatePath() const;
        void setTelemetryServerCertificatePath(const std::string& telemetryServerCertificatePath);

        int getPluginSendReceiveTimeout() const;
        void setPluginSendReceiveTimeout(int pluginSendReceiveTimeout);

        int getPluginConnectionTimeout() const;
        void setPluginConnectionTimeout(int pluginConnectionTimeout);

        bool operator==(const Config& rhs) const;
        bool operator!=(const Config& rhs) const;

        bool isValid() const;

    private:
        std::string m_server;
        std::string m_resourceRoot;
        std::string m_resourcePath;
        unsigned int m_port{};
        std::vector<std::string> m_headers;
        std::string m_verb;
        unsigned int m_interval{};
        std::vector<Proxy> m_proxies;
        std::vector<MessageRelay> m_messageRelays;
        unsigned int m_externalProcessWaitTime{};
        unsigned int m_externalProcessWaitRetries{};
        unsigned int m_maxJsonSize{};
        std::string m_telemetryServerCertificatePath;
        int m_pluginSendReceiveTimeout;
        int m_pluginConnectionTimeout;
    };
} // namespace Common::TelemetryConfigImpl
