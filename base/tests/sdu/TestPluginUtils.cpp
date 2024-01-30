// Copyright 2021-2023 Sophos Limited. All rights reserved.
#include "sdu/PluginUtils.h"
#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/MockFileSystem.h"

#include <gtest/gtest.h>
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"

class PluginUtilsTests : public LogInitializedTests
{

};
TEST_F(PluginUtilsTests, processUrlWillExtractAnddStoreValidDataCorrectly)
{
    RemoteDiagnoseImpl::PluginUtils::UrlData data =RemoteDiagnoseImpl::PluginUtils::processUrl("https://localhost/stuff/file.zip");
    EXPECT_EQ(data.filename,"file.zip");
    EXPECT_EQ(data.domain,"localhost");
    EXPECT_EQ(data.resourcePath,"stuff/file.zip");
    EXPECT_EQ(data.port,0);

}

TEST_F(PluginUtilsTests, processUrlwithPortWillValidDataIncludingPortCorrectly)
{
    RemoteDiagnoseImpl::PluginUtils::UrlData data =RemoteDiagnoseImpl::PluginUtils::processUrl("https://localhost:80/stuff/file.zip");
    EXPECT_EQ(data.filename,"file.zip");
    EXPECT_EQ(data.domain,"localhost");
    EXPECT_EQ(data.resourcePath,"stuff/file.zip");
    EXPECT_EQ(data.port,80);

}

TEST_F(PluginUtilsTests, processUrlThrowsWhenGivenInvalidPort)
{
    EXPECT_THROW(RemoteDiagnoseImpl::PluginUtils::processUrl("https://localhost:blah/stuff/file.zip"),std::runtime_error);
}

TEST_F(PluginUtilsTests, processUrlThrowWhenNotHTTPS)
{
    EXPECT_THROW(RemoteDiagnoseImpl::PluginUtils::processUrl("http://localhost/stuff/file.zip"),std::runtime_error);
}

TEST_F(PluginUtilsTests, getFinishedStatusDoesNotThrowWhenThereIsNoVersioniniFile)
{
    std::string expectedStatus {
    R"sophos(<?xml version="1.0" encoding="utf-8" ?><status version="" is_running="0" />)sophos" };
    std::string status = RemoteDiagnoseImpl::PluginUtils::getStatus(0);
    EXPECT_EQ(expectedStatus,status);
}

TEST_F(PluginUtilsTests, getFinishedStatusWillExtractVersionCorrectlyAndIsRunning)
{
    std::vector<std::string> contents ={"PRODUCT_NAME = SPL-Base-Component","PRODUCT_VERSION = 1.0.0","BUILD_DATE = 2021-06-11"};
    auto filesystemMock = std::make_unique<NiceMock<MockFileSystem>>();
    EXPECT_CALL(*filesystemMock, isFile(_)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readLines(_)).WillOnce(Return(contents));
    Tests::replaceFileSystem(std::move(filesystemMock));

    std::string expectedStatus {
        R"sophos(<?xml version="1.0" encoding="utf-8" ?><status version="1.0.0" is_running="1" />)sophos" };
    std::string status = RemoteDiagnoseImpl::PluginUtils::getStatus(1);
    EXPECT_EQ(expectedStatus,status);
}
TEST_F(PluginUtilsTests, getFinishedStatusWillExtractVersionCorrectlyAndIsNotRunning)
{
    std::vector<std::string> contents ={"PRODUCT_NAME = SPL-Base-Component","PRODUCT_VERSION = 1.0.0","BUILD_DATE = 2021-06-11"};
    auto filesystemMock = std::make_unique<NiceMock<MockFileSystem>>();
    EXPECT_CALL(*filesystemMock, isFile(_)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readLines(_)).WillOnce(Return(contents));
    Tests::replaceFileSystem(std::move(filesystemMock));

    std::string expectedStatus {
        R"sophos(<?xml version="1.0" encoding="utf-8" ?><status version="1.0.0" is_running="0" />)sophos" };
    std::string status = RemoteDiagnoseImpl::PluginUtils::getStatus(0);
    EXPECT_EQ(expectedStatus,status);
}
TEST_F(PluginUtilsTests, getFinishedStatusWillNotThrowOnInvalidProductVersion)
{
    std::vector<std::string> contents ={"PRODUCT_VERSION = NOTAVERSION"};
    auto filesystemMock = std::make_unique<NiceMock<MockFileSystem>>();
    EXPECT_CALL(*filesystemMock, isFile(_)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readLines(_)).WillOnce(Return(contents));
    Tests::replaceFileSystem(std::move(filesystemMock));

    EXPECT_NO_THROW(RemoteDiagnoseImpl::PluginUtils::getStatus(0));
}