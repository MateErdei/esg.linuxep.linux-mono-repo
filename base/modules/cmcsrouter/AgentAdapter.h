// Copyright 2022-2023 Sophos Limited. All rights reserved.
#pragma once

#include "IAdapter.h"

#include "Common/OSUtilities/ILocalIP.h"
#include "Common/OSUtilities/IPlatformUtils.h"
#include "cmcsrouter/Config.h"

#include <memory>
#include <vector>

#ifndef TEST_PUBLIC
#   define TEST_PUBLIC private
#endif

namespace MCS
{
    class AgentAdapter : public IAdapter
    {
    public:
        AgentAdapter();
        AgentAdapter(
            std::shared_ptr<Common::OSUtilities::IPlatformUtils> platformUtils,
            std::shared_ptr<Common::OSUtilities::ILocalIP> localIp);
        ~AgentAdapter() = default;

        [[nodiscard]] std::string getStatusXml(std::map<std::string, std::string>& configOptions) const override;
        std::string getStatusXml(
            std::map<std::string, std::string>& configOptions,
            const std::shared_ptr<Common::HttpRequests::IHttpRequester>& ptr) const override;

    TEST_PUBLIC:
        [[nodiscard]] std::string getPlatformStatus() const;

    private:
        [[nodiscard]] std::string getStatusHeader(const std::map<std::string, std::string>& configOptions) const;
        [[nodiscard]] std::string getCommonStatusXml(const std::map<std::string, std::string>& configOptions) const;
        [[nodiscard]] std::string getOptionalStatusValues(
            const std::map<std::string, std::string>& configOptions,
            const std::vector<std::string>& ip4Addresses,
            const std::vector<std::string>& ip6Addresses) const;
        [[nodiscard]] std::string getCloudPlatformsStatus(const std::shared_ptr<Common::HttpRequests::IHttpRequester>&
                                                              ptr) const; // Empty string if not cloud platform
        [[nodiscard]] std::string getStatusFooter() const;

        std::shared_ptr<Common::OSUtilities::IPlatformUtils> m_platformUtils;
        std::shared_ptr<Common::OSUtilities::ILocalIP> m_localIp;
    };
} // namespace MCS
