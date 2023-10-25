// Copyright 2023 Sophos Limited. All rights reserved.

#include "tests/Common/Helpers/MemoryAppender.h"

#include "Common/Policy/PolicyParseException.h"
#include "Common/Policy/MCSPolicy.h"

#include <gtest/gtest.h>

using namespace Common::Policy;

static std::string createMCSPolicy(const std::string& fixed_version)
{
    std::string mcspolicy = R"(<?xml version="1.0"?>
<policy xmlns:csc="com.sophos\msys\csc" type="mcs">
    <meta protocolVersion="1.1"/>
    <csc:Comp RevID="0e09e27cfc21f8d7510d562c56e34507711bdce48bfdfd3846965f130fef142a" policyType="25"/>
    <configuration xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd" xmlns:auto-ns1="com.sophos\mansys\policy">
        <deviceId>example-device-id</deviceId>
        <registrationToken>PolicyRegToken</registrationToken>
        <servers>
            <server>https://localhost:4443/mcs</server>
        </servers>
        <messageRelays>)" +
            fixed_version
        +
        R"(
        </messageRelays>
        <useSystemProxy>true</useSystemProxy>
        <useAutomaticProxy>true</useAutomaticProxy>
        <useDirect>true</useDirect>
        <randomSkewFactor>1</randomSkewFactor>
        <commandPollingDelay default="5"/>
        <flagsPollingInterval default="14400"/>
        <diagnosticTrailEnabled>true</diagnosticTrailEnabled>
        <policyChangeServers/>
        <pushServers/>
        <presignedUrlService>
            <url>https://mcs2-cloudstation-eu-west-1.prod.hydra.sophos.com/sophos/management/ep/presignedurls</url>
            <credentials>CCDmhj1cKco2P5YfLRaVBmF3zlDDaum+OsQ8tsmZCmEH4rzKvK2tm0P9Bl9plrQtGHoeBsno2Jal7bCsnnVVK4HEnhK2xNDA2UkJnrrBvYwINrgI0AytwyHvhYXvjYYJymQ=</credentials>
        </presignedUrlService>
    </configuration>
</policy>)";

    return mcspolicy;
}

namespace
{
    class TestMCSPolicy : public MemoryAppenderUsingTests
    {
    public:
        TestMCSPolicy() : MemoryAppenderUsingTests("Policy")
        {}
    };
}

TEST_F(TestMCSPolicy, extractMessageRelaySingle)
{
    auto fixed_version = R"(<messageRelay priority="1" address="1.1.1.1.1" port="90" id="2.2.2.2.2"/>)";

    MCSPolicy mcsObj{createMCSPolicy(fixed_version)};
    std::vector<MessageRelay> msgRelayVector = mcsObj.getMessageRelays();
    ASSERT_EQ(msgRelayVector.at(0).priority, "1");
    ASSERT_EQ(msgRelayVector.at(0).address, "1.1.1.1.1");
    ASSERT_EQ(msgRelayVector.at(0).port, "90");
    ASSERT_EQ(msgRelayVector.at(0).endpointId, "2.2.2.2.2");

}

TEST_F(TestMCSPolicy, extractMessageRelayMultiple)
{
    auto fixed_version = R"(<messageRelay priority="1" address="1.1.1.1.1" port="90" id="2.2.2.2.2"/>
    <messageRelay priority="10" address="10.10.10.10.10" port="900" id="20.20.20.20.20"/>)";
    MCSPolicy mcsObj{createMCSPolicy(fixed_version)};
    std::vector<MessageRelay> msgRelayVector = mcsObj.getMessageRelays();

    ASSERT_EQ(msgRelayVector.at(0).priority, "1");
    ASSERT_EQ(msgRelayVector.at(0).address, "1.1.1.1.1");
    ASSERT_EQ(msgRelayVector.at(0).port, "90");
    ASSERT_EQ(msgRelayVector.at(0).endpointId, "2.2.2.2.2");

    ASSERT_EQ(msgRelayVector.at(1).priority, "10");
    ASSERT_EQ(msgRelayVector.at(1).address, "10.10.10.10.10");
    ASSERT_EQ(msgRelayVector.at(1).port, "900");
    ASSERT_EQ(msgRelayVector.at(1).endpointId, "20.20.20.20.20");

}

TEST_F(TestMCSPolicy, extractMessageRelayEmptyMessageRelay)
{
    auto fixed_version = R"()";
    MCSPolicy mcsObj{createMCSPolicy(fixed_version)};
    std::vector<MessageRelay> msgRelayVector = mcsObj.getMessageRelays();

    ASSERT_TRUE(msgRelayVector.empty());
}

TEST_F(TestMCSPolicy, extractMessageRelayMalformed)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    auto fixed_version = R"(<<<<<<<<!3123>><)";

    EXPECT_THROW(MCSPolicy mcsObj{createMCSPolicy(fixed_version)}, Common::Policy::PolicyParseException);
    EXPECT_TRUE(appenderContains("Failed to parse policy: Error parsing xml: not well-formed (invalid token)"));
}

TEST_F(TestMCSPolicy, extractMessageRelayHostName)
{

    auto fixed_version = R"(<messageRelay priority="1" address="1.1.1.1.1" port="90" id="2.2.2.2.2"/>)";

    MCSPolicy mcsObj{createMCSPolicy(fixed_version)};
    std::vector<std::string> msgRelayVector = mcsObj.getMessageRelaysHostNames();
    ASSERT_EQ(msgRelayVector.at(0), "1.1.1.1.1:90");
}

TEST_F(TestMCSPolicy, sortMessageRelaysPriority)
{
    auto fixed_version = R"(<messageRelay priority="1" address="1.1.1.1.1" port="90" id="2.2.2.2.2"/>
    <messageRelay priority="10" address="10.10.10.10.10" port="900" id="20.20.20.20.20"/>
    <messageRelay priority="5" address="5.5.5.5.5" port="500" id="50.50.50.50.50"/>)";

    MCSPolicy mcsObj{createMCSPolicy(fixed_version)};

    std::vector<MessageRelay> relays = mcsObj.getMessageRelays();
    ASSERT_EQ("1", relays.at(0).priority);
    ASSERT_EQ("5", relays.at(1).priority);
    ASSERT_EQ("10", relays.at(2).priority);

}

