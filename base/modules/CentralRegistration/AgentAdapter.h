/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "CentralRegistration/IAdapter.h"
#include "Common/OSUtilitiesImpl/PlatformUtils.h"

#include <vector>

namespace CentralRegistrationImpl
{
    class AgentAdapter : public CentralRegistrationImpl::IAdapter
    {
    public:
        AgentAdapter();
        AgentAdapter(Common::OSUtilitiesImpl::PlatformUtils platformUtils);
        ~AgentAdapter() = default;

        [[nodiscard]] std::string getStatusXml() const override;

    private:
        [[nodiscard]] std::string getStatusHeader() const;
        [[nodiscard]] std::string getCommonStatusXml() const; // Options to be added as parameter later
        [[nodiscard]] std::string getOptionalStatusValues() const;
        //            std::string getCloudPlatformsStatus() const;  // Empty string if not cloud platform
        [[nodiscard]] std::string getPlatformStatus() const;
        [[nodiscard]] std::string getPolicyStatus() const;
        [[nodiscard]] std::string getStatusFooter() const;

        [[nodiscard]] std::string getSoftwareVersion() const;

        Common::OSUtilitiesImpl::PlatformUtils m_platformUtils;
    };
} // namespace CentralRegistrationImpl
