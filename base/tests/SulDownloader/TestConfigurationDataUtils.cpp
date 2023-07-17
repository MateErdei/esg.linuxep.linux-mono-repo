// Copyright 2023 Sophos All rights reserved.

#include "SulDownloader/suldownloaderdata/ConfigurationDataUtil.h"

#include <gtest/gtest.h>

using namespace Common::Policy;
using namespace SulDownloader::suldownloaderdata;

static Common::Policy::UpdateSettings getValidUpdateSettings()
{
    Common::Policy::UpdateSettings validSettings;
    validSettings.setSophosLocationURLs({"http://really_sophos.info"});
    validSettings.setCredentials({"username", "password"});
    validSettings.setPrimarySubscription({"RIGID", "Base", "tag", "fixed"});
    validSettings.setFeatures({"CORE"});
    return validSettings;
}


//checkIfShouldForceUpdate
TEST(TestConfigurationDataUtils, returnTrueIfESMVersionChanges)
{
    auto newSettings = getValidUpdateSettings();
    newSettings.setEsmVersion(ESMVersion("name", "token"));
    auto previousSettings = getValidUpdateSettings();
    EXPECT_TRUE(ConfigurationDataUtil::checkIfShouldForceUpdate(newSettings, previousSettings));
}

