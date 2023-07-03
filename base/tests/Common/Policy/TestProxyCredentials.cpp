// Copyright 2023 Sophos All rights reserved.

#include "modules/Common/Policy/ProxyCredentials.h"

#include <gtest/gtest.h>

TEST(TestProxyCredentials, deobfuscatePassword)
{
    // <proxy ProxyType="2" ProxyUserPassword="CCC4Fcz2iNaH44sdmqyLughrajL7svMPTbUZc/Q4c7yAtSrdM03lfO33xI0XKNU4IBY=" ProxyUserName="TestUser" ProxyPortNumber="8080" ProxyAddress="uk-abn-wpan-1.green.sophos" AllowLocalConfig="false"/>

    Common::Policy::ProxyCredentials creds{
        "TestUser", "CCC4Fcz2iNaH44sdmqyLughrajL7svMPTbUZc/Q4c7yAtSrdM03lfO33xI0XKNU4IBY=", "2"
    };
    EXPECT_EQ(creds.getDeobfuscatedPassword(), "Ch1pm0nk");
}
