/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Common/Policy/Proxy.h"

#include "Common/Policy/Credentials.h"
#include "Common/Policy/PolicyParseException.h"

#include "tests/Common/Helpers/LogInitializedTests.h"

#include <gtest/gtest.h>

using namespace Common::Policy;


TEST(Proxy, DefaultConstructorIsEmptyProxy)
{
    Proxy proxy;
    EXPECT_EQ(proxy.getUrl(), "");
    EXPECT_EQ(proxy.getProxyUrlAsSulRequires(), "");
    EXPECT_TRUE(proxy.empty());
}

TEST(Proxy, ShouldHandleSimpleProxy)
{
    Proxy proxy("10.10.10.10");
    EXPECT_EQ(proxy.getUrl(), "10.10.10.10");
    EXPECT_EQ(proxy.getProxyUrlAsSulRequires(), "http://10.10.10.10");
    EXPECT_FALSE(proxy.empty());
}

TEST(Proxy, ShouldHandleCorrectlyProxyUrlAsSulRequires)
{
    std::vector<std::pair<std::string, std::string>> input_expected = { { "10.10.10.10", "http://10.10.10.10" },
                                                                        { "10.10.10.10:50", "http://10.10.10.10:50" },
                                                                        { "myproxy.com", "http://myproxy.com" },
                                                                        { "https://myproxy.com",
                                                                          "https://myproxy.com" },
                                                                        { "http://myproxy.com", "http://myproxy.com" },
                                                                        { "noproxy:", "noproxy:" },
                                                                        { "environment:", "environment:" } };
    for (auto& cases : input_expected)
    {
        Proxy proxy(cases.first);
        EXPECT_EQ(proxy.getProxyUrlAsSulRequires(), cases.second);
    }
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
