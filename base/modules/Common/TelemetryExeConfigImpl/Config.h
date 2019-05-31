/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Constants.h"
#include "MessageRelay.h"
#include "Proxy.h"

#include <string>
#include <vector>

namespace Common::TelemetryExeConfigImpl
{
    class Config
    {
    public:
        Config();

        const std::string& getServer() const;
        void setServer(const std::string& server);

        const std::string& getResourceRoot() const;
        void setResourceRoot(const std::string& resourceRoot);

        unsigned int getPort() const;
        void setPort(unsigned int port);

        const std::vector<std::string>& getHeaders() const;
        void setHeaders(const std::vector<std::string>& headers);

        std::string getVerb() const;
        void setVerb(const std::string& verb);

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

        bool operator==(const Config& rhs) const;
        bool operator!=(const Config& rhs) const;

        bool isValid() const;

    private:
        std::string m_server;
        std::string m_resourceRoot;
        unsigned int m_port{};
        std::vector<std::string> m_headers;
        std::string m_verb;
        std::vector<Proxy> m_proxies;
        std::vector<MessageRelay> m_messageRelays;
        unsigned int m_externalProcessWaitTime{};
        unsigned int m_externalProcessWaitRetries{};
        unsigned int m_maxJsonSize{};
        std::string m_telemetryServerCertificatePath;
    };
} // namespace Telemetry::TelemetryExeConfigImpl
