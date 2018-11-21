/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/OSUtilities/ILocalIP.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace ::testing;



class MockILocalIP: public Common::OSUtilities::ILocalIP
{
public:
    static Common::OSUtilities::IPs buildIPsHelper( const std::string & ip4string)
    {
        Common::OSUtilities::IPs ips;
        ips.ip4collection.emplace_back( ip4string );
        return ips;
    }
    MOCK_CONST_METHOD0(getLocalIPs, Common::OSUtilities::IPs());
};

class FakeILocalIP: public Common::OSUtilities::ILocalIP
{

    Common::OSUtilities::IPs m_ips;
public:
    FakeILocalIP( const std::vector<std::string> & ip4s)
    {
        for( auto & ip4 : ip4s)
        {
            m_ips.ip4collection.emplace_back(Common::OSUtilities::IP4{ip4});
        }
    }
    Common::OSUtilities::IPs getLocalIPs() const {return m_ips; }
};
