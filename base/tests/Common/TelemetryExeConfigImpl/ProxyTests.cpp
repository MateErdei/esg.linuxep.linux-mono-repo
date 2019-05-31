/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <Common/TelemetryExeConfigImpl/Config.h>
#include <Common/TelemetryExeConfigImpl/Serialiser.h>

using ::testing::StrictMock;
using namespace Common::TelemetryExeConfigImpl;

class ProxyTests : public ::testing::Test
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

TEST_F(ProxyTests, passwordObfuscation) // NOLINT
{
    ASSERT_EQ(m_proxy.getDeobfuscatedPassword(), "password");
    ASSERT_EQ(m_proxy.getObfuscatedPassword(), "CCAcWWDAL1sCAV1YiHE20dTJIXMaTLuxrBppRLRbXgGOmQBrysz16sn7RuzXPaX6XHk=");
}

TEST_F(ProxyTests, proxyEqualitySameObject) // NOLINT
{
    ASSERT_EQ(m_proxy, m_proxy);
}

TEST_F(ProxyTests, proxyEqualityDiffObject) // NOLINT
{
    Proxy a = m_proxy;
    Proxy b = m_proxy;

    ASSERT_EQ(a, b);
}

TEST_F(ProxyTests, proxyNotEqualPort) // NOLINT
{
    Proxy a = m_proxy;
    Proxy b = m_proxy;
    b.setPort(1);
    ASSERT_NE(a, b);
}

TEST_F(ProxyTests, proxyNotEqualUrl) // NOLINT
{
    Proxy a = m_proxy;
    Proxy b = m_proxy;
    b.setUrl("blah");
    ASSERT_NE(a, b);
}

TEST_F(ProxyTests, proxyNotEqualAuth) // NOLINT
{
    Proxy a = m_proxy;
    Proxy b = m_proxy;
    b.setAuthentication(Proxy::Authentication::none);
    ASSERT_NE(a, b);
}

TEST_F(ProxyTests, proxyNotEqualUsername) // NOLINT
{
    Proxy a = m_proxy;
    Proxy b = m_proxy;
    b.setUsername("blah");
    ASSERT_NE(a, b);
}

TEST_F(ProxyTests, proxyNotEqualPassword) // NOLINT
{
    Proxy a = m_proxy;
    Proxy b = m_proxy;
    b.setPassword("blah");
    ASSERT_NE(a, b);
}

TEST_F(ProxyTests, proxyValid) // NOLINT
{
    ASSERT_TRUE(m_proxy.isValidProxy());
}

TEST_F(ProxyTests, proxyInvalidPort) // NOLINT
{
    Proxy p = m_proxy;
    p.setPort(5000000);
    ASSERT_FALSE(p.isValidProxy());
}

TEST_F(ProxyTests, proxyInvalidAuthNoneWithUsername) // NOLINT
{
    Proxy p = m_proxy;
    p.setAuthentication(Proxy::Authentication::none);
    p.setUsername("should not be set");
    p.setPassword("");
    ASSERT_FALSE(p.isValidProxy());
}

TEST_F(ProxyTests, proxyInvalidAuthNoneWithPassword) // NOLINT
{
    Proxy p = m_proxy;
    p.setAuthentication(Proxy::Authentication::none);
    p.setUsername("");
    p.setPassword("should not be set");
    ASSERT_FALSE(p.isValidProxy());
}

TEST_F(ProxyTests, proxyInvalidAuthBasicWithoutUsername) // NOLINT
{
    Proxy p = m_proxy;
    p.setAuthentication(Proxy::Authentication::none);
    p.setUsername("");
    p.setPassword("pw");
    ASSERT_FALSE(p.isValidProxy());
}

TEST_F(ProxyTests, proxyInvalidAuthBasicWithoutPassword) // NOLINT
{
    Proxy p = m_proxy;
    p.setAuthentication(Proxy::Authentication::none);
    p.setUsername("set");
    p.setPassword("");
    ASSERT_FALSE(p.isValidProxy());
}

TEST_F(ProxyTests, proxyInvalidAuthDigestWithoutUsername) // NOLINT
{
    Proxy p = m_proxy;
    p.setAuthentication(Proxy::Authentication::none);
    p.setUsername("");
    p.setPassword("pw");
    ASSERT_FALSE(p.isValidProxy());
}

TEST_F(ProxyTests, proxyInvalidAuthDigestWithoutPassword) // NOLINT
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
