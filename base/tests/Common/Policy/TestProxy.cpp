// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "modules/Common/Policy/Proxy.h"

#include "modules/Common/Policy/Credentials.h"
#include "modules/Common/Policy/PolicyParseException.h"

#include "tests/Common/Helpers/LogInitializedTests.h"

#include <gtest/gtest.h>

using namespace Common::Policy;

TEST(Proxy, DefaultConstructorIsEmptyProxy)
{
    Proxy proxy;
    EXPECT_EQ(proxy.getUrl(), "");
    EXPECT_TRUE(proxy.empty());
}

TEST(Proxy, NoProxyIsEmptyProxy)
{
    Proxy proxy(NoProxy);
    EXPECT_EQ(proxy.getUrl(), NoProxy);
    EXPECT_TRUE(proxy.empty());
}

TEST(Proxy, ShouldHandleSimpleProxy)
{
    Proxy proxy("10.10.10.10");
    EXPECT_EQ(proxy.getUrl(), "10.10.10.10");
    EXPECT_FALSE(proxy.empty());
}

class ProxyCredentialsTest: public LogOffInitializedTests{};
TEST_F(ProxyCredentialsTest, ShouldHandleType1Proxy)
{
    ProxyCredentials credential{ "user", "password", "1" };
    EXPECT_EQ(credential.getProxyType(), "1");
    EXPECT_EQ(credential.getDeobfuscatedPassword(), "password");
    EXPECT_EQ(credential.getPassword(), "password");
    EXPECT_EQ(credential.getUsername(), "user");
}

TEST_F(ProxyCredentialsTest, ShouldHandleType2Proxy)
{
    ProxyCredentials credential{ "user", "CCCj7sOF/IMdsPr1YxSIC0XjQcBmqy4kRtg7wwV0uCFxwzGl2qNaqk4lYs/6cQmFNLY=", "2" };
    EXPECT_EQ(credential.getProxyType(), "2");
    EXPECT_EQ(credential.getDeobfuscatedPassword(), "password");
    EXPECT_EQ(credential.getPassword(), "CCCj7sOF/IMdsPr1YxSIC0XjQcBmqy4kRtg7wwV0uCFxwzGl2qNaqk4lYs/6cQmFNLY=");
    EXPECT_EQ(credential.getUsername(), "user");
}

TEST_F(ProxyCredentialsTest, ShouldHandleType2SimpleProxy)
{
    ProxyCredentials credential{ "", "CCDN+JdsRVNd+yKFqQhrmdJ856KCCLHLQxEtgwG/tD5myvTrUk/kuALeUDhL4plxGvM=", "2" };
    EXPECT_EQ(credential.getProxyType(), "2");
    EXPECT_EQ(credential.getDeobfuscatedPassword(), "");
    EXPECT_EQ(credential.getPassword(), "CCDN+JdsRVNd+yKFqQhrmdJ856KCCLHLQxEtgwG/tD5myvTrUk/kuALeUDhL4plxGvM=");
    EXPECT_EQ(credential.getUsername(), "");
}

TEST_F(ProxyCredentialsTest, ShouldThrowOnInvalidCredential)
{
    std::vector<std::string> invalidPasswordsForEmptyUser = {
        { "CCCj7sOF/IMdsPr1YxSIC0XjQcBmqy4kRtg7wwV0uCFxwzGl2qNaqk4lYs/6cQmFNLY=" }, // non empty
        { "anyotherthing" },
    };
    for (auto& invalidPassword : invalidPasswordsForEmptyUser)
    {
        EXPECT_THROW(
            ProxyCredentials("", invalidPassword, "1"), Common::Policy::PolicyParseException);
    }
}
