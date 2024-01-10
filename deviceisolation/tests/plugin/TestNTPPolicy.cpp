// Copyright 2023 Sophos Limited. All rights reserved.

#include "pluginimpl/NTPPolicy.h"

#include "Common/XmlUtilities/AttributesMap.h"

#include "base/tests/Common/Helpers/MemoryAppender.h"

#include "pluginimpl/config.h"

#include <gtest/gtest.h>

namespace
{
    class TestNTPPolicy : public MemoryAppenderUsingTests
    {
    public:
        TestNTPPolicy() : MemoryAppenderUsingTests(PLUGIN_NAME)
        {}
    };
}

using namespace Plugin;

TEST_F(TestNTPPolicy, emptyString)
{
    try
    {
        NTPPolicy policy{""};
        FAIL() << "Empty policy should throw a NTPPolicyException";
    }
    catch (const NTPPolicyException& ex)
    {
        EXPECT_STREQ(ex.what(), "Unable to parse policy xml: ");
    }
}

TEST_F(TestNTPPolicy, minimalValidPolicy)
{
    NTPPolicy policy{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
</policy>
)SOPHOS"};
    EXPECT_EQ(policy.revId(), "ThisIsARevID");
    auto exclusions = policy.exclusions();
    EXPECT_EQ(exclusions.size(), 0);
}

TEST_F(TestNTPPolicy, multipleCscCompThrowsException)
{
    constexpr const auto* POLICY{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
</policy>
)SOPHOS"};
    EXPECT_THROW(NTPPolicy policy{POLICY};, std::runtime_error);
}

TEST_F(TestNTPPolicy, missingCscCompThrowsException)
{
    constexpr const auto* POLICY{R"SOPHOS(<?xml version="1.0"?>
<policy>
</policy>
)SOPHOS"};
    EXPECT_THROW(NTPPolicy policy{POLICY};, std::runtime_error);
}

TEST_F(TestNTPPolicy, missingPolicyThrowsException)
{
    constexpr const auto* POLICY{R"SOPHOS(<?xml version="1.0"?>)SOPHOS"};
    EXPECT_THROW(NTPPolicy policy{POLICY};, std::runtime_error);
}

TEST_F(TestNTPPolicy, rejectIncorrectPolicyType)
{
    constexpr const auto* POLICY{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="25" RevID="ThisIsARevID"/>
</policy>
)SOPHOS"};
    EXPECT_THROW(NTPPolicy policy{POLICY};, std::runtime_error);
}

TEST_F(TestNTPPolicy, multipleSelfIsolationElements)
{
    constexpr const auto* POLICY{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
    </selfIsolation>
    <selfIsolation>
    </selfIsolation>
</configuration>
</policy>
)SOPHOS"};
    EXPECT_THROW(NTPPolicy policy{POLICY};, std::runtime_error);
}

TEST_F(TestNTPPolicy, missingSelfIsolationWorks)
{
    constexpr const auto* POLICY{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
</policy>
)SOPHOS"};
    NTPPolicy policy{POLICY};
    auto exclusions = policy.exclusions();
    EXPECT_EQ(exclusions.size(), 0);
}

TEST_F(TestNTPPolicy, emptyExclusions)
{
    UsingMemoryAppender appender(*this);
    NTPPolicy policy{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
        <exclusions>
        </exclusions>
    </selfIsolation>
</configuration>
</policy>
)SOPHOS"};
    auto exclusions = policy.exclusions();
    EXPECT_EQ(exclusions.size(), 0);
    EXPECT_TRUE(appenderContains("Device Isolation using 0 exclusions"));
}


TEST_F(TestNTPPolicy, excludeHost)
{
    UsingMemoryAppender appender(*this);
    NTPPolicy policy{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
    <exclusions>
        <exclusion type="ip">
            <remoteAddress>10.10.10.10</remoteAddress>
        </exclusion>
    </exclusions>
    </selfIsolation>
</configuration>
</policy>
)SOPHOS"};
    auto exclusions = policy.exclusions();
    ASSERT_EQ(exclusions.size(), 1);
    auto exclusion = exclusions.at(0);
    ASSERT_FALSE(exclusion.remoteAddressesAndIpTypes().empty());
    auto addressAndIpType = exclusion.remoteAddressesAndIpTypes().at(0);
    EXPECT_EQ(addressAndIpType.first, "10.10.10.10");
    EXPECT_EQ(addressAndIpType.second, "ip");
    EXPECT_TRUE(exclusion.localPorts().empty());
    EXPECT_TRUE(exclusion.remotePorts().empty());
    EXPECT_EQ(exclusion.direction(), Plugin::IsolationExclusion::Direction::BOTH);
    EXPECT_TRUE(appenderContains("Device Isolation using 1 exclusions"));
}

TEST_F(TestNTPPolicy, singleExclusionEverything)
{
    UsingMemoryAppender appender(*this);
    NTPPolicy policy{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
        <exclusions>
            <exclusion type="ip">
            </exclusion>
        </exclusions>
    </selfIsolation>
</configuration>
</policy>
)SOPHOS"};
    auto exclusions = policy.exclusions();
    ASSERT_EQ(exclusions.size(), 1);
    auto exclusion = exclusions.at(0);
    EXPECT_EQ(exclusion.direction(), Plugin::IsolationExclusion::Direction::BOTH);
    EXPECT_TRUE(exclusion.localPorts().empty());
    EXPECT_TRUE(exclusion.remotePorts().empty());
    EXPECT_TRUE(exclusion.remoteAddressesAndIpTypes().empty());
    EXPECT_TRUE(appenderContains("Device Isolation using 1 exclusions"));
}

TEST_F(TestNTPPolicy, doubleExclusionEverything)
{
    UsingMemoryAppender appender(*this);
    NTPPolicy policy{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
        <exclusions>
            <exclusion type="ip"/>
            <exclusion type="ip"/>
        </exclusions>
    </selfIsolation>
</configuration>
</policy>
)SOPHOS"};
    auto exclusions = policy.exclusions();
    ASSERT_EQ(exclusions.size(), 2);
    for (const auto& exclusion : exclusions)
    {
        EXPECT_EQ(exclusion.direction(), Plugin::IsolationExclusion::Direction::BOTH);
        EXPECT_TRUE(exclusion.localPorts().empty());
        EXPECT_TRUE(exclusion.remotePorts().empty());
        EXPECT_TRUE(exclusion.remoteAddressesAndIpTypes().empty());
    }
    EXPECT_TRUE(appenderContains("Device Isolation using 2 exclusions"));
}

TEST_F(TestNTPPolicy, excludeIncoming)
{
    UsingMemoryAppender appender(*this);
    NTPPolicy policy{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
        <exclusions>
            <exclusion type="ip">
                <direction>in</direction>
            </exclusion>
        </exclusions>
    </selfIsolation>
</configuration>
</policy>
)SOPHOS"};
    auto exclusions = policy.exclusions();
    ASSERT_EQ(exclusions.size(), 1);
    auto exclusion = exclusions.at(0);
    EXPECT_EQ(exclusion.direction(), Plugin::IsolationExclusion::Direction::IN);
    EXPECT_TRUE(exclusion.localPorts().empty());
    EXPECT_TRUE(exclusion.remotePorts().empty());
    EXPECT_TRUE(exclusion.remoteAddressesAndIpTypes().empty());
    EXPECT_TRUE(appenderContains("Device Isolation using 1 exclusions"));
}

//Direction Exclusion Parameter
TEST_F(TestNTPPolicy, absentDirectionParameterResultsInDirectionBoth)
{
    UsingMemoryAppender appender(*this);
    NTPPolicy policy{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
        <exclusions>
            <exclusion type="ip">
                <remoteAddress>12.12.12.12</remoteAddress>
                <localPort>80</localPort>
                <remotePort>22</remotePort>
            </exclusion>
        </exclusions>
    </selfIsolation>
</configuration>
</policy>
)SOPHOS"};
    auto exclusions = policy.exclusions();
    ASSERT_EQ(exclusions.size(), 1);
    auto exclusion = exclusions.at(0);
    EXPECT_EQ(exclusion.direction(), Plugin::IsolationExclusion::Direction::BOTH);
    EXPECT_TRUE(appenderContains("Device Isolation using 1 exclusions"));
}

TEST_F(TestNTPPolicy, directionsIsNotString)
{
    UsingMemoryAppender appender(*this);
    NTPPolicy policy{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
        <exclusions>
            <exclusion type="ip">
                <direction>124</direction>
                <remoteAddress>12.12.12.12</remoteAddress>
                <localPort>80</localPort>
            </exclusion>
        </exclusions>
    </selfIsolation>
</configuration>
</policy>
)SOPHOS"};
    auto exclusions = policy.exclusions();
    ASSERT_EQ(exclusions.size(), 0);
    EXPECT_TRUE(appenderContains("Invalid direction entry: 124, expect in or out"));
    EXPECT_TRUE(appenderContains("Device Isolation using 0 exclusions"));
}


TEST_F(TestNTPPolicy, multipleDirectionsInExclusion)
{
    UsingMemoryAppender appender(*this);
    NTPPolicy policy{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
        <exclusions>
            <exclusion type="ip">
                <direction>foo</direction>
                <direction>bar</direction>
            </exclusion>
        </exclusions>
    </selfIsolation>
</configuration>
</policy>
)SOPHOS"};
    auto exclusions = policy.exclusions();
    ASSERT_EQ(exclusions.size(), 0);
    EXPECT_TRUE(appenderContains("Invalid number of direction parameters: 2"));
    EXPECT_TRUE(appenderContains("Device Isolation using 0 exclusions"));
}


TEST_F(TestNTPPolicy, multipleExclusionsOneValidDirectionOneInvalidDirection)
{
    UsingMemoryAppender appender(*this);
    NTPPolicy policy{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
        <exclusions>
            <exclusion type="ip">
                <direction>foo</direction>
                <remoteAddress>12.12.12.12</remoteAddress>
                <localPort>80</localPort>
            </exclusion>
            <exclusion type="ip">
                <direction>in</direction>
                <remoteAddress>124.124.124.124</remoteAddress>
                <localPort>100</localPort>
            </exclusion>
        </exclusions>
    </selfIsolation>
</configuration>
</policy>
)SOPHOS"};
    auto exclusions = policy.exclusions();
    ASSERT_EQ(exclusions.size(), 1);
    auto exclusion = exclusions.at(0);
    EXPECT_EQ(exclusion.direction(), Plugin::IsolationExclusion::Direction::IN);
    EXPECT_TRUE(appenderContains("Invalid direction entry: foo, expect in or out"));
    EXPECT_TRUE(appenderContains("Device Isolation using 1 exclusions"));
}


//RemoteAddress Exclusion Parameter
TEST_F(TestNTPPolicy, noRemoteAddress)
{
    UsingMemoryAppender appender(*this);
    NTPPolicy policy{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
    <exclusions>
        <exclusion type="ip">
            <direction>in</direction>
            <localPort>22</localPort>
            <remotePort>22</remotePort>
        </exclusion>
    </exclusions>
    </selfIsolation>
</configuration>
</policy>
)SOPHOS"};
    auto exclusions = policy.exclusions();
    ASSERT_EQ(exclusions.size(), 1);
    auto exclusion = exclusions.at(0);
    EXPECT_TRUE(exclusion.remoteAddressesAndIpTypes().empty());
    EXPECT_TRUE(appenderContains("Device Isolation using 1 exclusions"));
}

TEST_F(TestNTPPolicy, remoteAddressIsMalformed)
{
    UsingMemoryAppender appender(*this);

    NTPPolicy policy{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
    <exclusions>
        <exclusion type="ip">
            <direction>in</direction>
            <remoteAddress>not an ip</remoteAddress>
            <localPort>80</localPort>
        </exclusion>
    </exclusions>
    </selfIsolation>
</configuration>
</policy>
)SOPHOS"};
    auto exclusions = policy.exclusions();
    ASSERT_EQ(exclusions.size(), 0);

    auto expectStr = R"(Invalid exclusion remote address: Invalid value "not an ip" for policy/configuration/selfIsolation/exclusions/exclusion/remoteAddress)";
    EXPECT_TRUE(appenderContains(expectStr));
    EXPECT_TRUE(appenderContains("Device Isolation using 0 exclusions"));
}


TEST_F(TestNTPPolicy, remoteAddressIsMalformedInOneExclusionAndValidInAnother)
{
    UsingMemoryAppender appender(*this);

    NTPPolicy policy{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
    <exclusions>
        <exclusion type="ip">
            <direction>in</direction>
            <remoteAddress>not an ip</remoteAddress>
            <localPort>66</localPort>
        </exclusion>
        <exclusion type="ip">
            <direction>out</direction>
            <remoteAddress>1.2.3.4</remoteAddress>
            <localPort>80</localPort>
        </exclusion>
    </exclusions>
    </selfIsolation>
</configuration>
</policy>
)SOPHOS"};
    auto exclusions = policy.exclusions();
    ASSERT_EQ(exclusions.size(), 1);
    EXPECT_EQ(exclusions.at(0).remoteAddressesAndIpTypes().at(0).first, "1.2.3.4");
    EXPECT_EQ(exclusions.at(0).remoteAddressesAndIpTypes().at(0).second, "ip");
    auto expectStr = R"(Invalid exclusion remote address: Invalid value "not an ip" for policy/configuration/selfIsolation/exclusions/exclusion/remoteAddress)";
    EXPECT_TRUE(appenderContains(expectStr));
    EXPECT_TRUE(appenderContains("Device Isolation using 1 exclusions"));
}

TEST_F(TestNTPPolicy, allowMultipleRemoteAddresses)
{
    UsingMemoryAppender appender(*this);

    NTPPolicy policy{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
    <exclusions>
        <exclusion type="ip">
            <remoteAddress>1.2.3.4</remoteAddress>
            <remoteAddress>5.6.7.8</remoteAddress>
        </exclusion>
    </exclusions>
    </selfIsolation>
</configuration>
</policy>
)SOPHOS"};
    auto exclusions = policy.exclusions();
    ASSERT_EQ(exclusions.size(), 1);
    auto exclusion = exclusions.at(0);
    IsolationExclusion::address_iptype_list_t addresses { {"1.2.3.4", "ip"}, {"5.6.7.8", "ip"} };
    EXPECT_EQ(exclusion.remoteAddressesAndIpTypes(), addresses);
    EXPECT_TRUE(appenderContains("Device Isolation using 1 exclusions"));
}

TEST_F(TestNTPPolicy, ignoreAllRemoteAddressesInExclusionIfOneIsInvalid)
{
    UsingMemoryAppender appender(*this);

    NTPPolicy policy{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
    <exclusions>
        <exclusion type="ip">
            <remoteAddress>1.2.3.4</remoteAddress>
            <remoteAddress>notaddress</remoteAddress>
        </exclusion>
    </exclusions>
    </selfIsolation>
</configuration>
</policy>
)SOPHOS"};
    auto exclusions = policy.exclusions();
    ASSERT_EQ(exclusions.size(), 0);
    auto expectStr = R"(Invalid exclusion remote address: Invalid value "notaddress" for policy/configuration/selfIsolation/exclusions/exclusion/remoteAddress)";
    EXPECT_TRUE(appenderContains(expectStr));
    EXPECT_TRUE(appenderContains("Device Isolation using 0 exclusions"));
}

//Local Port Exclusion Parameter
TEST_F(TestNTPPolicy, excludeLocalPort)
{
    UsingMemoryAppender appender(*this);

    NTPPolicy policy{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
    <exclusions>
        <exclusion type="ip">
            <localPort>22</localPort>
        </exclusion>
    </exclusions>
    </selfIsolation>
</configuration>
</policy>
)SOPHOS"};
    auto exclusions = policy.exclusions();
    ASSERT_EQ(exclusions.size(), 1);
    auto exclusion = exclusions.at(0);
    ASSERT_FALSE(exclusion.localPorts().empty());
    EXPECT_EQ(exclusion.localPorts(), IsolationExclusion::port_list_t {"22"});
    EXPECT_TRUE(appenderContains("Device Isolation using 1 exclusions"));
}

TEST_F(TestNTPPolicy, noLocalPort)
{
    UsingMemoryAppender appender(*this);
    NTPPolicy policy{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
    <exclusions>
        <exclusion type="ip">
            <direction>in</direction>
            <remoteAddress>11.11.11.11</remoteAddress>
            <remotePort>22</remotePort>
        </exclusion>
    </exclusions>
    </selfIsolation>
</configuration>
</policy>
)SOPHOS"};
    auto exclusions = policy.exclusions();
    ASSERT_EQ(exclusions.size(), 1);
    auto exclusion = exclusions.at(0);
    EXPECT_TRUE(exclusion.localPorts().empty());
    EXPECT_TRUE(appenderContains("Device Isolation using 1 exclusions"));
}


TEST_F(TestNTPPolicy, localPortIsNotNumeric)
{
    UsingMemoryAppender appender(*this);
    NTPPolicy policy{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
    <exclusions>
        <exclusion type="ip">
            <direction>in</direction>
            <remoteAddress>12.12.12.12</remoteAddress>
            <localPort>not a number!</localPort>
        </exclusion>
    </exclusions>
    </selfIsolation>
</configuration>
</policy>
)SOPHOS"};
    auto exclusions = policy.exclusions();
    ASSERT_EQ(exclusions.size(), 0);
    auto expectStr = R"(Invalid exclusion local port: Invalid value "not a number!" for policy/configuration/selfIsolation/exclusions/exclusion/localPort)";
    EXPECT_TRUE(appenderContains(expectStr));
    EXPECT_TRUE(appenderContains("Device Isolation using 0 exclusions"));
}

TEST_F(TestNTPPolicy, allowMultipleLocalPorts)
{
    UsingMemoryAppender appender(*this);

    NTPPolicy policy{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
    <exclusions>
        <exclusion type="ip">
            <localPort>22</localPort>
            <localPort>44</localPort>
        </exclusion>
    </exclusions>
    </selfIsolation>
</configuration>
</policy>
)SOPHOS"};
    auto exclusions = policy.exclusions();
    ASSERT_EQ(exclusions.size(), 1);
    auto exclusion = exclusions.at(0);
    IsolationExclusion::port_list_t ports { "22", "44" };
    EXPECT_EQ(exclusion.localPorts(), ports);

    EXPECT_TRUE(appenderContains("Device Isolation using 1 exclusions"));
}

TEST_F(TestNTPPolicy, ignoreAllLocalPortsIfOneIsInvalid)
{
    UsingMemoryAppender appender(*this);
    NTPPolicy policy{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
    <exclusions>
        <exclusion type="ip">
            <localPort>22</localPort>
            <localPort>not a port</localPort>
        </exclusion>
    </exclusions>
    </selfIsolation>
</configuration>
</policy>
)SOPHOS"};
    auto exclusions = policy.exclusions();
    ASSERT_EQ(exclusions.size(), 0);
    auto expectStr = R"(Invalid exclusion local port: Invalid value "not a port" for policy/configuration/selfIsolation/exclusions/exclusion/localPort)";
    EXPECT_TRUE(appenderContains(expectStr));
    EXPECT_TRUE(appenderContains("Device Isolation using 0 exclusions"));
}

TEST_F(TestNTPPolicy, allowOneExclusionIfPortIsValidIgnoreOtherWithInvalidPort)
{
    UsingMemoryAppender appender(*this);
    NTPPolicy policy{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
    <exclusions>
        <exclusion type="ip">
            <localPort>not a port</localPort>
        </exclusion>
        <exclusion type="ip">
            <localPort>22</localPort>
        </exclusion>
    </exclusions>
    </selfIsolation>
</configuration>
</policy>
)SOPHOS"};
    auto exclusions = policy.exclusions();
    ASSERT_EQ(exclusions.size(), 1);
    auto exclusion = exclusions.at(0);
    EXPECT_EQ(exclusion.localPorts(), IsolationExclusion::port_list_t { "22" });
    auto expectStr = R"(Invalid exclusion local port: Invalid value "not a port" for policy/configuration/selfIsolation/exclusions/exclusion/localPort)";
    EXPECT_TRUE(appenderContains(expectStr));
    EXPECT_TRUE(appenderContains("Device Isolation using 1 exclusions"));
}

//Remote Port Exclusion Parameter
TEST_F(TestNTPPolicy, remotePortIsNotNumeric)
{
    UsingMemoryAppender appender(*this);
    NTPPolicy policy{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
    <exclusions>
        <exclusion type="ip">
            <direction>in</direction>
            <remoteAddress>3.3.3.3</remoteAddress>
            <remotePort>not a number!</remotePort>
        </exclusion>
    </exclusions>
    </selfIsolation>
</configuration>
</policy>
)SOPHOS"};
    ASSERT_EQ(policy.exclusions().size(), 0);
    auto expectStr = R"(Invalid exclusion remote port: Invalid value "not a number!" for policy/configuration/selfIsolation/exclusions/exclusion/remotePort)";
    EXPECT_TRUE(appenderContains(expectStr));
    EXPECT_TRUE(appenderContains("Device Isolation using 0 exclusions"));
}

TEST_F(TestNTPPolicy, excludeRemotePort)
{
    UsingMemoryAppender appender(*this);
    NTPPolicy policy{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
    <exclusions>
        <exclusion type="ip">
            <remotePort>22</remotePort>
        </exclusion>
    </exclusions>
    </selfIsolation>
</configuration>
</policy>
)SOPHOS"};
    auto exclusions = policy.exclusions();
    ASSERT_EQ(exclusions.size(), 1);
    auto exclusion = exclusions.at(0);
    EXPECT_EQ( exclusion.remotePorts(), IsolationExclusion::port_list_t {"22"});
    EXPECT_TRUE(appenderContains("Device Isolation using 1 exclusions"));
}

TEST_F(TestNTPPolicy, allowMultipleRemotePorts)
{
    UsingMemoryAppender appender(*this);

    NTPPolicy policy{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
    <exclusions>
        <exclusion type="ip">
            <remotePort>22</remotePort>
            <remotePort>44</remotePort>
        </exclusion>
    </exclusions>
    </selfIsolation>
</configuration>
</policy>
)SOPHOS"};
    auto exclusions = policy.exclusions();
    ASSERT_EQ(exclusions.size(), 1);
    auto exclusion = exclusions.at(0);
    IsolationExclusion::port_list_t ports { "22", "44" };
    EXPECT_EQ(exclusion.remotePorts(), ports);

    EXPECT_TRUE(appenderContains("Device Isolation using 1 exclusions"));
}

TEST_F(TestNTPPolicy, ignoreAllRemotePortsIfOneIsInvalid)
{
    UsingMemoryAppender appender(*this);
    NTPPolicy policy{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
    <exclusions>
        <exclusion type="ip">
            <remotePort>22</remotePort>
            <remotePort>not a port</remotePort>
        </exclusion>
    </exclusions>
    </selfIsolation>
</configuration>
</policy>
)SOPHOS"};
    auto exclusions = policy.exclusions();
    ASSERT_EQ(exclusions.size(), 0);
    auto expectStr = R"(Invalid exclusion remote port: Invalid value "not a port" for policy/configuration/selfIsolation/exclusions/exclusion/remotePort)";
    EXPECT_TRUE(appenderContains(expectStr));
    EXPECT_TRUE(appenderContains("Device Isolation using 0 exclusions"));
}

TEST_F(TestNTPPolicy, allowOneExclusionIfRemotePortIsValidIgnoreOtherWithInvalidRemotePort)
{
    UsingMemoryAppender appender(*this);
    NTPPolicy policy{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
    <exclusions>
        <exclusion type="ip">
            <remotePort>not a port</remotePort>
        </exclusion>
        <exclusion type="ip">
            <remotePort>22</remotePort>
        </exclusion>
    </exclusions>
    </selfIsolation>
</configuration>
</policy>
)SOPHOS"};
    auto exclusions = policy.exclusions();
    ASSERT_EQ(exclusions.size(), 1);
    auto exclusion = exclusions.at(0);
    EXPECT_EQ(exclusion.remotePorts(), IsolationExclusion::port_list_t { "22" });
    auto expectStr = R"(Invalid exclusion remote port: Invalid value "not a port" for policy/configuration/selfIsolation/exclusions/exclusion/remotePort)";
    EXPECT_TRUE(appenderContains(expectStr));
    EXPECT_TRUE(appenderContains("Device Isolation using 1 exclusions"));
}

TEST_F(TestNTPPolicy, noRemotePort)
{
    UsingMemoryAppender appender(*this);
    NTPPolicy policy{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
    <exclusions>
        <exclusion type="ip">
            <direction>in</direction>
            <remoteAddress>4.4.4.4</remoteAddress>
            <localPort>22</localPort>
        </exclusion>
    </exclusions>
    </selfIsolation>
</configuration>
</policy>
)SOPHOS"};
    auto exclusions = policy.exclusions();
    ASSERT_EQ(exclusions.size(), 1);
    auto exclusion = exclusions.at(0);
    EXPECT_TRUE(exclusion.remotePorts().empty());
    EXPECT_TRUE(appenderContains("Device Isolation using 1 exclusions"));
}

//Exclusion Type
TEST_F(TestNTPPolicy, ignoreExclusionOfNonIpType)
{
    UsingMemoryAppender appender(*this);
    NTPPolicy policy{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
    <exclusions>
        <exclusion type="foobar">
            <remotePort>22</remotePort>
        </exclusion>
    </exclusions>
    </selfIsolation>
</configuration>
</policy>
)SOPHOS"};
    auto exclusions = policy.exclusions();
    EXPECT_EQ(exclusions.size(), 0);
    EXPECT_TRUE(appenderContains("Device Isolation using 0 exclusions"));
}

TEST_F(TestNTPPolicy, ignoreExclusionOfNonStringType)
{
    auto policyStr = R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
    <exclusions>
        <exclusion type=1234>
            <remotePort>22</remotePort>
        </exclusion>
    </exclusions>
    </selfIsolation>
</configuration>
</policy>
)SOPHOS";

    try
    {
        NTPPolicy policy{policyStr};
        FAIL() << "Did not throw with invalid exclusion type";
    }
    catch (const NTPPolicyException& ex)
    {
        std::stringstream ss;
        ss << "Unable to parse policy xml: " << policyStr;
        EXPECT_EQ(ex.what(), ss.str());
    }
}

TEST_F(TestNTPPolicy, ignoreExclusionWithoutType)
{
    UsingMemoryAppender appender(*this);
    NTPPolicy policy{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
    <exclusions>
        <exclusion>
            <remotePort>22</remotePort>
        </exclusion>
    </exclusions>
    </selfIsolation>
</configuration>
</policy>
)SOPHOS"};
    auto exclusions = policy.exclusions();
    EXPECT_EQ(exclusions.size(), 0);
    EXPECT_TRUE(appenderContains("Device Isolation using 0 exclusions"));
}

//Full Policy
TEST_F(TestNTPPolicy, examplePolicy)
{
    constexpr const auto* POLICY=R"SOPHOS(<?xml version="1.0"?>
<policy xmlns="com.sophos\mansys\policy"
        xmlns:xsd="http://www.w3.org/2001/XMLSchema"
        xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
  <csc:Comp xmlns:csc="com.sophos\msys\csc" policyType="24" RevID="{{revId}}"/>
  <configuration xmlns="http://www.sophos.com/xml/msys/NetworkThreatProtection.xsd">
    <mtdEnabled>{{mtdEnabled}}</mtdEnabled>
    <enabled>{{c2Detections}}</enabled>
    <connectionTracking>{{connectionTracking}}</connectionTracking>
    <exclusions>
      <!-- from Global Settings > Global Exclusions > File or Folder (WIndows) -->
      <filePathSet>
        <filePath>{{exclusion}}</filePath>
        <!-- filePath element repeated as many times as needed -->
      </filePathSet>
    </exclusions>
    <selfIsolation>
      <enabled>{{selfIsolationEnabled}}</enabled>
      <exclusions>
        <!-- from Global Settings > Global Exclusions > Device isolation -->
        <exclusion type="{{exclusionType}}">
          <direction>{{exclusionDirection}}</direction> <!-- optional -->
          <remoteAddress>{{exclusionAddress}}</remoteAddress> <!-- optional -->
          <!-- WINEP-15806 will allow remoteAddress element to repeat as many times as needed -->
          <localPort>{{exclusionLocalPort}}</localPort> <!-- optional -->
          <!-- WINEP-15806 will allow localPort element to repeat as many times as needed -->
          <remotePort>{{exclusionRemotePort}}</remotePort> <!-- optional -->
          <!-- WINEP-15806 will allow remotePort element to repeat as many times as needed -->
        </exclusion>
        <!-- exclusion element repeated as many times as needed -->
      </exclusions>
    </selfIsolation>
    <ips> <!-- Coming in 2019 -->
      <enabled>{{ipsEnabled}}</enabled>
      <exclusions>
        <!-- from Global Settings > Global Exclusions > Network communications -->
        <exclusion type="ip">
          <direction>{{exclusionDirection}}</direction> <!-- optional -->
          <remoteAddress>{{exclusionAddress}}</remoteAddress>
          <localPort>{{exclusionLocalPort}}</localPort> <!-- optional -->
          <remotePort>{{exclusionRemotePort}}</remotePort> <!-- optional -->
        </exclusion>
        <!-- exclusion element repeated as many times as needed -->
      </exclusions>
    </ips>
  </configuration>
</policy>)SOPHOS";
    NTPPolicy policy{POLICY};
    EXPECT_EQ(policy.revId(), "{{revId}}");
}

TEST_F(TestNTPPolicy, examplePolicyFromCentral)
{
    constexpr const auto examplePolicy = R"SOPHOS(<?xml version="1.0"?>
<policy xmlns="com.sophos\mansys\policy" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
    <csc:Comp xmlns:csc="com.sophos\msys\csc" policyType="24" RevID="fb3fb6e2889efd2e694ab1c64b0c488f0c09b29017835676802a19da94cecb15"/>
    <configuration xmlns="http://www.sophos.com/xml/msys/NetworkThreatProtection.xsd">
        <enabled>true</enabled>
        <connectionTracking>true</connectionTracking>
        <exclusions>
            <filePathSet>
                <filePath>/tmp/eicar.com</filePath>
                <filePath>/test1/</filePath>
            </filePathSet>
        </exclusions>
        <selfIsolation>
            <enabled>false</enabled>
            <exclusions>
                <exclusion type="ip">
                    <direction>in</direction>
                    <localPort>443</localPort>
                </exclusion>
                <exclusion type="ip">
                    <direction>out</direction>
                    <remoteAddress>192.168.1.9</remoteAddress>
                </exclusion>
                <exclusion type="ip">
                    <remoteAddress>192.168.1.1</remoteAddress>
                    <localPort>22</localPort>
                </exclusion>
            </exclusions>
        </selfIsolation>
        <ips>
            <enabled>false</enabled>
            <exclusions/>
        </ips>
    </configuration>
</policy>)SOPHOS";
    NTPPolicy policy{examplePolicy};
    EXPECT_EQ(policy.revId(), "fb3fb6e2889efd2e694ab1c64b0c488f0c09b29017835676802a19da94cecb15");

    EXPECT_EQ(policy.exclusions().size(), 3);

    EXPECT_EQ(policy.exclusions().at(0).direction(), Plugin::IsolationExclusion::IN);
    EXPECT_EQ(policy.exclusions().at(0).localPorts().at(0), "443");

    EXPECT_EQ(policy.exclusions().at(1).direction(), Plugin::IsolationExclusion::OUT);
    EXPECT_EQ(policy.exclusions().at(1).remoteAddressesAndIpTypes().at(0).first, "192.168.1.9");
    EXPECT_EQ(policy.exclusions().at(1).remoteAddressesAndIpTypes().at(0).second, "ip");

    EXPECT_EQ(policy.exclusions().at(2).remoteAddressesAndIpTypes().at(0).first, "192.168.1.1");
    EXPECT_EQ(policy.exclusions().at(2).remoteAddressesAndIpTypes().at(0).second, "ip");
    EXPECT_EQ(policy.exclusions().at(2).localPorts().at(0), "22");
}
