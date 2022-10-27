// Copyright 2022, Sophos Limited.  All rights reserved.

#include <pluginimpl/ThreatDatabase.h>
#include "tests/common/LogInitializedTests.h"

#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFileSystem.h"

namespace
{
    class TestThreatDatabase : public LogInitializedTests
    {




    };
}

TEST_F(TestThreatDatabase, initdatabase)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists("/path/persist-threatDatabase")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/path/persist-threatDatabase")).WillOnce(Return("{}"));
    EXPECT_CALL(*filesystemMock, writeFile("/path/persist-threatDatabase","{}"));

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase("/path");
}
TEST_F(TestThreatDatabase, initdatabasehandlesEmptyString)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists("/path/persist-threatDatabase")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/path/persist-threatDatabase")).WillOnce(Return(""));
    EXPECT_CALL(*filesystemMock, writeFile("/path/persist-threatDatabase","{}"));

    EXPECT_NO_THROW(Plugin::ThreatDatabase("/path"));
}
