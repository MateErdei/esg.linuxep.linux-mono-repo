/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "OSUtilities/IPlatformUtils.h"
#include <sys/utsname.h>

namespace Common
{
    namespace OSUtilitiesImpl
    {
        class PlatformUtils : public Common::OSUtilities::IPlatformUtils
        {
        public:
            PlatformUtils();
            ~ PlatformUtils() = default;

            [[nodiscard]] std::string getHostname() const override;
            [[nodiscard]] std::string getPlatform() const override;
            [[nodiscard]] std::string getVendor() const override;
            [[nodiscard]] std::string getArchitecture() const override;
            [[nodiscard]] std::string getOsName() const override;
            [[nodiscard]] std::string getKernelVersion() const override;
            [[nodiscard]] std::string getOsMajorVersion() const override;
            [[nodiscard]] std::string getOsMinorVersion() const override;
            [[nodiscard]] std::string getDomainname() const override;
//            std::string getCloudPlatform() const override;

        private:
            /**
             * Gets the basic platform information
             * @return a utsname struct containing platform information
             */
            utsname getUtsname() const;
            void populateVendorDetails();

            std::string m_vendor;
            std::string m_osName;
            std::string m_osMajorVersion;
            std::string m_osMinorVersion;
        };

    } // namespace OSUtilitiesImpl
} // namespace Common