/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <Common/OSUtilities/IDnsLookup.h>

#include <memory>
namespace Common
{
    namespace OSUtilitiesImpl
    {
        class DnsLookupImpl : public Common::OSUtilities::IDnsLookup
        {
        public:
            Common::OSUtilities::IPs lookup(const std::string&) const override;
        };

        /** To be used in tests only */
        using IDnsLookupPtr = std::unique_ptr<Common::OSUtilities::IDnsLookup>;
        void replaceDnsLookup(IDnsLookupPtr);
        void restoreDnsLookup();

    } // namespace OSUtilitiesImpl
} // namespace Common