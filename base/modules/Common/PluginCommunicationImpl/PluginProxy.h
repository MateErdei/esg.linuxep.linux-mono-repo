// Copyright 2018-2023 Sophos Limited. All rights reserved.
#pragma once

#include "AppIdCollection.h"

#include "Common/PluginCommunication/IPluginProxy.h"
#include "Common/PluginProtocol/MessageBuilder.h"
#include "Common/ZeroMQWrapper/IReadWrite.h"
#include "Common/ZeroMQWrapper/ISocketRequester.h"
#include "Common/ZeroMQWrapper/ISocketRequesterPtr.h"

namespace Common::PluginCommunicationImpl
{
    class PluginProxy : virtual public PluginCommunication::IPluginProxy
    {
    public:
        PluginProxy(Common::ZeroMQWrapper::ISocketRequesterPtr socketRequester, const std::string& pluginName);
        void applyNewPolicy(const std::string& appId, const std::string& policyXml) override;
        void queueAction(const std::string& appId, const std::string& actionXml, const std::string& correlationId)
            override;
        std::vector<Common::PluginApi::StatusInfo> getStatus() override;
        std::string getTelemetry() override;
        std::string getHealth() override;

        void setPolicyAppIds(const std::vector<std::string>& appIds) override;
        void setActionAppIds(const std::vector<std::string>& appIds) override;

        void setStatusAppIds(const std::vector<std::string>& appIds) override;

        /**
         *
         * @param appId
         * @return true if the plugin is interested in the appId given
         */
        bool hasPolicyAppId(const std::string& appId) override;
        bool hasActionAppId(const std::string& appId) override;
        bool hasStatusAppId(const std::string& appId) override;

        /**
         * Get the plugin's name
         * @return
         */
        std::string name() override { return m_name; }

        void setServiceHealth(bool serviceHealth) override;
        void setThreatServiceHealth(bool threatServiceHealth) override;
        void setDisplayPluginName(const std::string& displayPluginName) override;
        bool getServiceHealth() const override;
        bool getThreatServiceHealth() const override;
        std::string getDisplayPluginName() const override;

    private:
        Common::PluginProtocol::DataMessage getReply(const Common::PluginProtocol::DataMessage& request) const;
        Common::ZeroMQWrapper::ISocketRequesterPtr m_socket;
        Common::PluginProtocol::MessageBuilder m_messageBuilder;
        AppIdCollection m_appIdCollection;
        std::string m_name;
        bool m_serviceHealth;
        bool m_threatServiceHealth;
        std::string m_displayPluginName;
    };
} // namespace Common::PluginCommunicationImpl
