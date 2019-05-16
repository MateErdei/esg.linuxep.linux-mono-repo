/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <modules/Telemetry/TelemetryConfig/Config.h>
#include <modules/Telemetry/TelemetryConfig/TelemetryConfigSerialiser.h>

using ::testing::StrictMock;
using namespace Telemetry::TelemetryConfig;

class TelemetryProxyTest : public ::testing::Test
{
public:
    Proxy m_proxy;

    void SetUp() override
    {
        m_proxy.setUrl("proxy");
        m_proxy.setPort(789);
        m_proxy.setAuthentication(MessageRelay::Authentication::basic);
        m_proxy.setUsername("proxyuser");
        m_proxy.setPassword("CCAcWWDAL1sCAV1YiHE20dTJIXMaTLuxrBppRLRbXgGOmQBrysz16sn7RuzXPaX6XHk=");
    }
};

TEST_F(TelemetryProxyTest, passwordObfuscation) // NOLINT
{
    ASSERT_EQ(m_proxy.getDeobfuscatedPassword(), "password");
    ASSERT_EQ(m_proxy.getObfuscatedPassword(), "CCAcWWDAL1sCAV1YiHE20dTJIXMaTLuxrBppRLRbXgGOmQBrysz16sn7RuzXPaX6XHk=");
}

TEST_F(TelemetryProxyTest, proxyEqualitySameObject) // NOLINT
{
    ASSERT_EQ(m_proxy, m_proxy);
}

TEST_F(TelemetryProxyTest, proxyEqualityDiffObject) // NOLINT
{
    Proxy a = m_proxy;
    Proxy b = m_proxy;

    ASSERT_EQ(a, b);
}

TEST_F(TelemetryProxyTest, proxyNotEqualPort) // NOLINT
{
    Proxy a = m_proxy;
    Proxy b = m_proxy;
    b.setPort(1);
    ASSERT_NE(a, b);
}

TEST_F(TelemetryProxyTest, proxyNotEqualUrl) // NOLINT
{
    Proxy a = m_proxy;
    Proxy b = m_proxy;
    b.setUrl("blah");
    ASSERT_NE(a, b);
}

TEST_F(TelemetryProxyTest, proxyNotEqualAuth) // NOLINT
{
    Proxy a = m_proxy;
    Proxy b = m_proxy;
    b.setAuthentication(Proxy::Authentication::none);
    ASSERT_NE(a, b);
}

TEST_F(TelemetryProxyTest, proxyNotEqualUsername) // NOLINT
{
    Proxy a = m_proxy;
    Proxy b = m_proxy;
    b.setUsername("blah");
    ASSERT_NE(a, b);
}

TEST_F(TelemetryProxyTest, proxyNotEqualPassword) // NOLINT
{
    Proxy a = m_proxy;
    Proxy b = m_proxy;
    b.setPassword("blah");
    ASSERT_NE(a, b);
}

TEST_F(TelemetryProxyTest, proxyValid) // NOLINT
{
    ASSERT_TRUE(m_proxy.isValidProxy());
}

TEST_F(TelemetryProxyTest, proxyInvalidPort) // NOLINT
{
    Proxy p = m_proxy;
    p.setPort(5000000);
    ASSERT_FALSE(p.isValidProxy());
}

TEST_F(TelemetryProxyTest, proxyInvalidAuthNoneWithUsername) // NOLINT
{
    Proxy p = m_proxy;
    p.setAuthentication(Proxy::Authentication::none);
    p.setUsername("should not be set");
    p.setPassword("");
    ASSERT_FALSE(p.isValidProxy());
}

TEST_F(TelemetryProxyTest, proxyInvalidAuthNoneWithPassword) // NOLINT
{
    Proxy p = m_proxy;
    p.setAuthentication(Proxy::Authentication::none);
    p.setUsername("");
    p.setPassword("should not be set");
    ASSERT_FALSE(p.isValidProxy());
}

TEST_F(TelemetryProxyTest, proxyInvalidAuthBasicWithoutUsername) // NOLINT
{
    Proxy p = m_proxy;
    p.setAuthentication(Proxy::Authentication::none);
    p.setUsername("");
    p.setPassword("pw");
    ASSERT_FALSE(p.isValidProxy());
}

TEST_F(TelemetryProxyTest, proxyInvalidAuthBasicWithoutPassword) // NOLINT
{
    Proxy p = m_proxy;
    p.setAuthentication(Proxy::Authentication::none);
    p.setUsername("set");
    p.setPassword("");
    ASSERT_FALSE(p.isValidProxy());
}

TEST_F(TelemetryProxyTest, proxyInvalidAuthDigestWithoutUsername) // NOLINT
{
    Proxy p = m_proxy;
    p.setAuthentication(Proxy::Authentication::none);
    p.setUsername("");
    p.setPassword("pw");
    ASSERT_FALSE(p.isValidProxy());
}

TEST_F(TelemetryProxyTest, proxyInvalidAuthDigestWithoutPassword) // NOLINT
{
    Proxy p = m_proxy;
    p.setAuthentication(Proxy::Authentication::none);
    p.setUsername("set");
    p.setPassword("");
    ASSERT_FALSE(p.isValidProxy());
}

TEST(ProxyTest, proxyEquality) // NOLINT
{
    Proxy a;
    Proxy b;
    a.setPort(2);
    ASSERT_NE(a, b);

    b.setPort(2);
    ASSERT_EQ(a, b);
}
