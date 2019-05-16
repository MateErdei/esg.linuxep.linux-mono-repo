/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "MessageRelay.h"
#include "Proxy.h"
#include "Constants.h"

#include "Common/HttpSenderImpl/RequestConfig.h"

#include <string>
#include <vector>

namespace Telemetry::TelemetryConfig
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

        Common::HttpSenderImpl::RequestType getVerb() const;
        void setVerb(Common::HttpSenderImpl::RequestType verb);

        const std::vector<Proxy>& getProxies() const;
        void setProxies(const std::vector<Proxy>& proxies);

        const std::vector<MessageRelay>& getMessageRelays() const;
        void setMessageRelays(const std::vector<MessageRelay>& messageRelays);

        unsigned int getExternalProcessTimeout() const;
        void setExternalProcessTimeout(unsigned int externalProcessTimeout);

        unsigned int getExternalProcessRetries() const;
        void setExternalProcessRetries(unsigned int externalProcessRetries);

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
        Common::HttpSenderImpl::RequestType m_verb;
        std::string m_certPath;
        std::vector<Proxy> m_proxies;
        std::vector<MessageRelay> m_messageRelays;
        unsigned int m_externalProcessTimeout{};
        unsigned int m_externalProcessRetries{};
        unsigned int m_maxJsonSize{};
        std::string m_telemetryServerCertificatePath;
    };
} // namespace Telemetry::TelemetryConfig
