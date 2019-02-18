/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <Common/OSUtilities/ILocalIP.h>

#include <memory>
namespace Common
{
    namespace OSUtilitiesImpl
    {
        class LocalIPImpl : public Common::OSUtilities::ILocalIP
        {
        public:
            Common::OSUtilities::IPs getLocalIPs() const override;
        };

        /** To be used in tests only */
        using ILocalIPPtr = std::unique_ptr<Common::OSUtilities::ILocalIP>;
        void replaceLocalIP(ILocalIPPtr);
        void restoreLocalIP();
    } // namespace OSUtilitiesImpl
} // namespace Common