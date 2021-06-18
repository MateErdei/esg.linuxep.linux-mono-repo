/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <sdu/PluginUtils.h>
#include <gtest/gtest.h>

TEST(PluginUtilsTests, processUrlWillExtractAnddStoreValidDataCorrectly) // NOLINT
{
    RemoteDiagnoseImpl::PluginUtils::UrlData data =RemoteDiagnoseImpl::PluginUtils::processUrl("https://localhost/stuff/file.zip");
    EXPECT_EQ(data.filename,"file.zip");
    EXPECT_EQ(data.domain,"localhost");
    EXPECT_EQ(data.resourcePath,"stuff/file.zip");
    EXPECT_EQ(data.port,0);

}

TEST(PluginUtilsTests, processUrlwithPortWillValidDataIncludingPortCorrectly) // NOLINT
{
    RemoteDiagnoseImpl::PluginUtils::UrlData data =RemoteDiagnoseImpl::PluginUtils::processUrl("https://localhost:80/stuff/file.zip");
    EXPECT_EQ(data.filename,"file.zip");
    EXPECT_EQ(data.domain,"localhost");
    EXPECT_EQ(data.resourcePath,"stuff/file.zip");
    EXPECT_EQ(data.port,80);

}

TEST(PluginUtilsTests, processUrlThrowsWhenGivenInvalidPort) // NOLINT
{
    EXPECT_THROW(RemoteDiagnoseImpl::PluginUtils::processUrl("https://localhost:blah/stuff/file.zip"),std::runtime_error);
}

TEST(PluginUtilsTests, processUrlThrowWhenNotHTTPS) // NOLINT
{
    EXPECT_THROW(RemoteDiagnoseImpl::PluginUtils::processUrl("http://localhost/stuff/file.zip"),std::runtime_error);
}
