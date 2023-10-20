// Copyright 2023 Sophos Limited. All rights reserved.

#include "sophos_threat_detector/sophosthreatdetectorimpl/ProxySettings.h"

// SPL Test
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileNotFoundException.h"

// test headers
#include "tests/common/TestSpecificDirectory.h"

#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFileSystem.h"

// 3rd party
#include <gtest/gtest.h>

namespace
{
    class TestProxySettings : public TestSpecificDirectory
    {
    public:
        TestProxySettings() :  TestSpecificDirectory("SophosThreatDetectorImpl")
        {}
    };
}

TEST_F(TestProxySettings, missing_file)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto* filesystemMock = new NaggyMock<MockFileSystem>();

    auto path = Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    EXPECT_CALL(*filesystemMock, readFile(path, _)).WillOnce(Throw(Common::FileSystem::IFileNotFoundException("FOO")));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };

    EXPECT_NO_THROW(sspl::sophosthreatdetectorimpl::setProxyFromConfigFile());
    EXPECT_TRUE(appenderContains("LiveProtection will use direct SXL4 connections"));
}

TEST_F(TestProxySettings, empty_file)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto* filesystemMock = new NaggyMock<MockFileSystem>();

    auto path = Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    EXPECT_CALL(*filesystemMock, readFile(path, _)).WillOnce(Return(""));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };

    EXPECT_NO_THROW(sspl::sophosthreatdetectorimpl::setProxyFromConfigFile());
    EXPECT_TRUE(appenderContains("LiveProtection will use direct SXL4 connections"));
}

TEST_F(TestProxySettings, syntax_error)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto* filesystemMock = new NaggyMock<MockFileSystem>();

    auto path = Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    EXPECT_CALL(*filesystemMock, readFile(path, _)).WillOnce(Return("{\"stuff\")"));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };

    EXPECT_NO_THROW(sspl::sophosthreatdetectorimpl::setProxyFromConfigFile());
    EXPECT_TRUE(appenderContains("LiveProtection will use direct SXL4 connections"));
}

TEST_F(TestProxySettings, missing_key)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto* filesystemMock = new NaggyMock<MockFileSystem>();

    auto path = Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    EXPECT_CALL(*filesystemMock, readFile(path, _)).WillOnce(Return(R"({"stuff":"foo"})"));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };

    EXPECT_NO_THROW(sspl::sophosthreatdetectorimpl::setProxyFromConfigFile());
    EXPECT_TRUE(appenderContains("LiveProtection will use direct SXL4 connections"));
}

TEST_F(TestProxySettings, proxy_is_integer)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto* filesystemMock = new NaggyMock<MockFileSystem>();

    auto path = Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    EXPECT_CALL(*filesystemMock, readFile(path, _)).WillOnce(Return("{\"proxy\":12}"));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };

    EXPECT_NO_THROW(sspl::sophosthreatdetectorimpl::setProxyFromConfigFile());
    EXPECT_TRUE(appenderContains("LiveProtection will use direct SXL4 connections"));
}

TEST_F(TestProxySettings, contains_proxy)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto* filesystemMock = new NaggyMock<MockFileSystem>();

    auto path = Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    EXPECT_CALL(*filesystemMock, readFile(path, _)).WillOnce(Return(R"({"proxy":"localhost:8080"})"));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };

    EXPECT_NO_THROW(sspl::sophosthreatdetectorimpl::setProxyFromConfigFile());
    EXPECT_TRUE(appenderContains("LiveProtection will use http://localhost:8080 for SXL4 connections"));
    const char* proxy = ::getenv("https_proxy");
    ASSERT_NE(proxy, nullptr);
    EXPECT_STREQ(proxy, "http://localhost:8080");
}

