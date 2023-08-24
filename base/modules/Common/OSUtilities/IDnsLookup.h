/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "IIPUtils.h"

#include "Common/SystemCallWrapper/ISystemCallWrapper.h"

namespace Common
{
    namespace OSUtilities
    {
        class IDnsLookup
        {
        public:
            virtual ~IDnsLookup() = default;
            /**
             * Return the ips associated with the given server address.
             * @param uri
             * @param sysCallWrapper
             * @return the ips associated with the given server address or throw exception if it can not get the dns
             * resolution of the given address.
             */
            virtual IPs lookup(const std::string& uri, Common::SystemCallWrapper::ISystemCallWrapperSharedPtr sysCallWrapper) const = 0;
        };

        /**
         * Return a BORROWED pointer to a static IDnsLookup instance.
         *
         * Do not delete this yourself.
         *
         * @return BORROWED IDnsLookup pointer
         */
        IDnsLookup* dnsLookup();

    } // namespace OSUtilities
} // namespace Common