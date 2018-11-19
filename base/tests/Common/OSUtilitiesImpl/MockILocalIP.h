/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/OSUtilities/ILocalIP.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace ::testing;

Common::OSUtilities::IPs buildIPsHelper( const std::string & ip4string)
{
    Common::OSUtilities::IPs ips;
    ips.ip4collection.emplace_back( ip4string );
    return ips;
}

class MockILocalIP: public Common::OSUtilities::ILocalIP
{
public:

    MOCK_CONST_METHOD0(getLocalIPs, Common::OSUtilities::IPs());
};
