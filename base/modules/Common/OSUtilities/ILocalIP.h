/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "IIPUtils.h"
namespace Common
{
    namespace OSUtilities
    {
        class ILocalIP
        {
        public:
            virtual ~ILocalIP() = default;
            /**
             * Return the ips associated with the local machine.
             * @return
             */
            virtual IPs getLocalIPs() const = 0;
        };


        /**
        * Return a BORROWED pointer to a static ILocalIP instance.
        *
        * Do not delete this yourself.
        *
        * @return BORROWED ILocalIP pointer
        */
        ILocalIP *localIP();


    }
}