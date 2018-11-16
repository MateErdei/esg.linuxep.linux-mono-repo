/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once
#include <Common/OSUtilities/IIPUtils.h>
#include <net/if.h>

namespace Common
{
    namespace OSUtilitiesImpl
    {

        Common::OSUtilities::IP4 fromIP4(struct sockaddr * ifa_addr);

        Common::OSUtilities::IP6 fromIP6(struct sockaddr * ifa_addr);



    }

}