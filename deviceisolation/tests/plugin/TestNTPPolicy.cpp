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
    EXPECT_THROW(NTPPolicy policy{""};, Common::XmlUtilities::XmlUtilitiesException);
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
    constexpr const auto* POLICY{R"SOPHOS(<?xml version="1.0"?>
)SOPHOS"};
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
    EXPECT_TRUE(waitForLog("Device Isolation using 0 exclusions"));
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
    EXPECT_TRUE(exclusion.remoteAddresses().empty());
    EXPECT_TRUE(waitForLog("Device Isolation using 1 exclusions"));
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
        EXPECT_TRUE(exclusion.remoteAddresses().empty());
    }
    EXPECT_TRUE(waitForLog("Device Isolation using 2 exclusions"));
}

TEST_F(TestNTPPolicy, excludeIncoming)
{
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
    EXPECT_TRUE(exclusion.remoteAddresses().empty());
}

TEST_F(TestNTPPolicy, noDirection)
{
    NTPPolicy policy{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
        <exclusions>
            <exclusion type="ip">
                <remoteAddress>REMOTE_ADDRESS</remoteAddress>
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
    EXPECT_EQ(exclusion.direction(), Plugin::IsolationExclusion::Direction::BOTH);
}

TEST_F(TestNTPPolicy, multipleDirectionsInExclusion)
{
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
    ASSERT_EQ(exclusions.size(), 1);
    auto exclusion = exclusions.at(0);
    EXPECT_EQ(exclusion.direction(), Plugin::IsolationExclusion::Direction::BOTH);
    EXPECT_TRUE(exclusion.localPorts().empty());
    EXPECT_TRUE(exclusion.remotePorts().empty());
    EXPECT_TRUE(exclusion.remoteAddresses().empty());
}

TEST_F(TestNTPPolicy, excludeHost)
{
    NTPPolicy policy{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
    <exclusions>
        <exclusion type="ip">
            <remoteAddress>REMOTE_ADDRESS</remoteAddress>
        </exclusion>
    </exclusions>
    </selfIsolation>
</configuration>
</policy>
)SOPHOS"};
    auto exclusions = policy.exclusions();
    ASSERT_EQ(exclusions.size(), 1);
    auto exclusion = exclusions.at(0);
    ASSERT_FALSE(exclusion.remoteAddresses().empty());
    auto address = exclusion.remoteAddresses().at(0);
    EXPECT_EQ(address, "REMOTE_ADDRESS");
    EXPECT_TRUE(exclusion.localPorts().empty());
    EXPECT_TRUE(exclusion.remotePorts().empty());
}

TEST_F(TestNTPPolicy, noRemoteAddress)
{
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
    EXPECT_TRUE(exclusion.remoteAddresses().empty());
}

TEST_F(TestNTPPolicy, excludeLocalPort)
{
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
    auto port = exclusion.localPorts().at(0);
    EXPECT_EQ(port, "22");
    EXPECT_TRUE(exclusion.remotePorts().empty());
    EXPECT_TRUE(exclusion.remoteAddresses().empty());
}

TEST_F(TestNTPPolicy, noLocalPort)
{
    NTPPolicy policy{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
    <exclusions>
        <exclusion type="ip">
            <direction>in</direction>
            <remoteAddress>REMOTE_ADDRESS</remoteAddress>
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
}

TEST_F(TestNTPPolicy, excludeRemotePort)
{
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
    ASSERT_FALSE(exclusion.remotePorts().empty());
    auto port = exclusion.remotePorts().at(0);
    EXPECT_EQ(port, "22");
    EXPECT_TRUE(exclusion.localPorts().empty());
    EXPECT_TRUE(exclusion.remoteAddresses().empty());
}

TEST_F(TestNTPPolicy, noRemotePort)
{
    NTPPolicy policy{R"SOPHOS(<?xml version="1.0"?>
<policy>
<csc:Comp xmlns:csc="com.sophos\\msys\\csc" policyType="24" RevID="ThisIsARevID"/>
<configuration>
    <selfIsolation>
    <exclusions>
        <exclusion type="ip">
            <direction>in</direction>
            <remoteAddress>REMOTE_ADDRESS</remoteAddress>
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
}

TEST_F(TestNTPPolicy, ignoreExclusionOfNonIpType)
{
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
}

TEST_F(TestNTPPolicy, ignoreExclusionWithoutType)
{
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
}

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