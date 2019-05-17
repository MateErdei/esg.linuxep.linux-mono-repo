/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <SulDownloader/suldownloaderdata/Credentials.h>
#include <SulDownloader/suldownloaderdata/Proxy.h>
#include <SulDownloader/suldownloaderdata/SulDownloaderException.h>
#include <gtest/gtest.h>

using namespace SulDownloader;
using namespace SulDownloader::suldownloaderdata;

TEST(Proxy, DefaultConstructorIsEmptyProxy) // NOLINT
{
    Proxy proxy;
    EXPECT_EQ(proxy.getUrl(), "");
    EXPECT_EQ(proxy.getProxyUrlAsSulRequires(), "");
    EXPECT_TRUE(proxy.empty());
}

TEST(Proxy, ShouldHandleSimpleProxy) // NOLINT
{
    Proxy proxy("10.10.10.10");
    EXPECT_EQ(proxy.getUrl(), "10.10.10.10");
    EXPECT_EQ(proxy.getProxyUrlAsSulRequires(), "http://10.10.10.10");
    EXPECT_FALSE(proxy.empty());
}

TEST(Proxy, ShouldHandleCorrectlyProxyUrlAsSulRequires) // NOLINT
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

TEST(ProxyCredentials, ShouldHandleType1Proxy)
{
    ProxyCredentials credential{ "user", "password", "1" };
    EXPECT_EQ(credential.getProxyType(), "1");
    EXPECT_EQ(credential.getDeobfuscatedPassword(), "password");
    EXPECT_EQ(credential.getPassword(), "password");
    EXPECT_EQ(credential.getUsername(), "user");
}

TEST(ProxyCredentials, ShouldHandleType2Proxy)
{
    ProxyCredentials credential{ "user", "CCCj7sOF/IMdsPr1YxSIC0XjQcBmqy4kRtg7wwV0uCFxwzGl2qNaqk4lYs/6cQmFNLY=", "2" };
    EXPECT_EQ(credential.getProxyType(), "2");
    EXPECT_EQ(credential.getDeobfuscatedPassword(), "password");
    EXPECT_EQ(credential.getPassword(), "CCCj7sOF/IMdsPr1YxSIC0XjQcBmqy4kRtg7wwV0uCFxwzGl2qNaqk4lYs/6cQmFNLY=");
    EXPECT_EQ(credential.getUsername(), "user");
}

TEST(ProxyCredentials, ShouldHandleType2SimpleProxy)
{
    ProxyCredentials credential{ "", "CCDN+JdsRVNd+yKFqQhrmdJ856KCCLHLQxEtgwG/tD5myvTrUk/kuALeUDhL4plxGvM=", "2" };
    EXPECT_EQ(credential.getProxyType(), "2");
    EXPECT_EQ(credential.getDeobfuscatedPassword(), "");
    EXPECT_EQ(credential.getPassword(), "CCDN+JdsRVNd+yKFqQhrmdJ856KCCLHLQxEtgwG/tD5myvTrUk/kuALeUDhL4plxGvM=");
    EXPECT_EQ(credential.getUsername(), "");
}

TEST(ProxyCredentials, ShouldThrowOnInvalidCredential)
{
    std::vector<std::string> invalidPasswordsForEmptyUser = {
        { "CCCj7sOF/IMdsPr1YxSIC0XjQcBmqy4kRtg7wwV0uCFxwzGl2qNaqk4lYs/6cQmFNLY=" }, // non empty
        { "anyotherthing" },
    };
    for (auto& invalidPassword : invalidPasswordsForEmptyUser)
    {
        EXPECT_THROW(
            ProxyCredentials("", invalidPassword, "1"), SulDownloader::suldownloaderdata::SulDownloaderException);
    }
}
