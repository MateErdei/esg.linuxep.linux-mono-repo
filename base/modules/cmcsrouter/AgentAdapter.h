/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "IAdapter.h"
#include <Common/OSUtilities/IPlatformUtils.h>

#include <memory>
#include <vector>

namespace MCS
{
    class AgentAdapter : public IAdapter
    {
    public:
        AgentAdapter();
        AgentAdapter(std::shared_ptr<Common::OSUtilities::IPlatformUtils> platformUtils);
        ~AgentAdapter() = default;

        [[nodiscard]] std::string getStatusXml(std::map<std::string, std::string>& configOptions) const override;

    private:
        [[nodiscard]] std::string getStatusHeader() const;
        [[nodiscard]] std::string getCommonStatusXml(std::map<std::string, std::string>& configOptions) const; // Options to be added as parameter later
        [[nodiscard]] std::string getOptionalStatusValues(std::map<std::string, std::string>& configOptions) const;
        [[nodiscard]] std::string getCloudPlatformsStatus(std::map<std::string, std::string>& configOptions) const;  // Empty string if not cloud platform
        [[nodiscard]] std::string getPlatformStatus() const;
        [[nodiscard]] std::string getPolicyStatus() const;
        [[nodiscard]] std::string getStatusFooter() const;

        [[nodiscard]] std::string getSoftwareVersion() const;

        std::shared_ptr<Common::OSUtilities::IPlatformUtils> m_platformUtils;
    };
}
