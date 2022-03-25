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
            std::string getHostname() const override;
            std::string getPlatform() const override;
            std::string getVendor() const override;
            std::string getArchitecture() const override;

        private:
            void checkUtsname() const;
        };

    } // namespace OSUtilitiesImpl
} // namespace Common