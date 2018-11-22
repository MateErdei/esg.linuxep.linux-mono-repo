/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "IIPUtils.h"
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
             * @return the ips associated with the given server address or throw exception if it can not get the dns resolution of the given address.
             */
            virtual IPs lookup(const std::string & uri) const = 0 ;
        };

        /**
        * Return a BORROWED pointer to a static IDnsLookup instance.
        *
        * Do not delete this yourself.
        *
        * @return BORROWED IDnsLookup pointer
        */
        IDnsLookup *dnsLookup();

    }
}