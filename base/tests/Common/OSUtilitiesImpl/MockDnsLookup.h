/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <Common/OSUtilities/IDnsLookup.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <unordered_map>

using namespace ::testing;

class MockIDnsLookup : public Common::OSUtilities::IDnsLookup
{
public:
    MOCK_CONST_METHOD1(lookup, Common::OSUtilities::IPs(const std::string&));
};

class FakeIDnsLookup : public Common::OSUtilities::IDnsLookup
{
    std::unordered_map<std::string, Common::OSUtilities::IPs> m_dns;

public:
    FakeIDnsLookup() = default;
    void addMap(const std::string& serveruri, const std::vector<std::string>& ip4s)
    {
        Common::OSUtilities::IPs ips;
        for (auto& ip4 : ip4s)
        {
            ips.ip4collection.emplace_back(Common::OSUtilities::IP4{ ip4 });
        }
        m_dns[serveruri] = ips;
    }
    Common::OSUtilities::IPs lookup(const std::string& uri) const
    {
        auto found = m_dns.find(uri);
        if (found != m_dns.end())
        {
            return found->second;
        }
        throw std::runtime_error("Not present");
    }
};
