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