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

TEST_F(TestThreatDatabase, initDatabaseWorksWhenNoFile)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists("/path/persist-threatDatabase")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, writeFile("/path/persist-threatDatabase","{}"));

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase("/path");
}

TEST_F(TestThreatDatabase, initDatabaseHandlesEmptyStringInFile)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists("/path/persist-threatDatabase")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/path/persist-threatDatabase")).WillOnce(Return(""));
    EXPECT_CALL(*filesystemMock, writeFile("/path/persist-threatDatabase","{}"));

    EXPECT_NO_THROW(Plugin::ThreatDatabase("/path"));
}

TEST_F(TestThreatDatabase, initDatabaseHandlesEmptyDatabaseInFile)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists("/path/persist-threatDatabase")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/path/persist-threatDatabase")).WillOnce(Return("{}"));
    EXPECT_CALL(*filesystemMock, writeFile("/path/persist-threatDatabase","{}"));

    EXPECT_NO_THROW(Plugin::ThreatDatabase("/path"));
}

TEST_F(TestThreatDatabase, initDatabaseHandlesMalformedJsonStringInFile)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists("/path/persist-threatDatabase")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/path/persist-threatDatabase")).WillOnce(Return("{:stuf}"));
    EXPECT_CALL(*filesystemMock, writeFile("/path/persist-threatDatabase","{}"));

    EXPECT_NO_THROW(Plugin::ThreatDatabase("/path"));
}

TEST_F(TestThreatDatabase, DatabaseLoadsAndSavesCorrectly)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists("/path/persist-threatDatabase")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/path/persist-threatDatabase")).WillOnce(Return("{\"threatid\":[\"threatid\"]}"));
    EXPECT_CALL(*filesystemMock, writeFile("/path/persist-threatDatabase","{\"threatid\":[\"threatid\"]}"));

    EXPECT_NO_THROW(Plugin::ThreatDatabase("/path"));
}

TEST_F(TestThreatDatabase, isDatabaseEmptyReturnsTrueOnEmptyDatabase)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists("/path/persist-threatDatabase")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/path/persist-threatDatabase")).WillOnce(Return("{}"));
    EXPECT_CALL(*filesystemMock, writeFile("/path/persist-threatDatabase","{}"));

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase("/path");
    EXPECT_TRUE(database.isDatabaseEmpty());
}

TEST_F(TestThreatDatabase, isDatabaseEmptyReturnsFalseOnNonEmptyDatabase)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists("/path/persist-threatDatabase")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/path/persist-threatDatabase")).WillOnce(Return("{}"));
    EXPECT_CALL(*filesystemMock, writeFile("/path/persist-threatDatabase","{\"threat\":[\"threat\"]}"));

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase("/path");
    database.addThreat("threat","threat");
    EXPECT_FALSE(database.isDatabaseEmpty());
}

TEST_F(TestThreatDatabase, addThreatToDatabase)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists("/path/persist-threatDatabase")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/path/persist-threatDatabase")).WillOnce(Return("{\"threatid\":[\"threatid\"]}"));
    EXPECT_CALL(*filesystemMock, writeFile("/path/persist-threatDatabase","{\"threatID\":[\"correlationid2\"],\"threatid\":[\"threatid\"]}"));

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase("/path");
    database.addThreat("threatID","correlationid2");
}

TEST_F(TestThreatDatabase, addCorrelationIDToDatabase)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists("/path/persist-threatDatabase")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/path/persist-threatDatabase")).WillOnce(Return("{\"threatid\":[\"threatid\"]}"));
    EXPECT_CALL(*filesystemMock, writeFile("/path/persist-threatDatabase","{\"threatid\":[\"threatid\",\"correlationid\"]}"));

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase("/path");
    database.addThreat("threatid","correlationid");
}

TEST_F(TestThreatDatabase, DatabaseLoadsAndSavesHandlesUnexpectedStructureNewCorrelationID)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists("/path/persist-threatDatabase")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/path/persist-threatDatabase")).WillOnce(Return("{\"threatid\":1000}"));
    EXPECT_CALL(*filesystemMock, writeFile("/path/persist-threatDatabase","{\"threatid\":[\"correlationid2\"]}"));

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase("/path");
    database.addThreat("threatid","correlationid2");
}

TEST_F(TestThreatDatabase, DatabaseLoadsAndSavesHandlesUnexpectedStructureNewThreatID)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists("/path/persist-threatDatabase")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/path/persist-threatDatabase")).WillOnce(Return("{\"threatID1\":1000}"));
    EXPECT_CALL(*filesystemMock, writeFile("/path/persist-threatDatabase","{\"threatID2\":[\"correlationid2\"]}"));

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase("/path");
    database.addThreat("threatID2","correlationid2");
}

TEST_F(TestThreatDatabase, removeCorrelationIDRemovesThreatWhenNoMoreCorrelationIDsForThreatID)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists("/path/persist-threatDatabase")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/path/persist-threatDatabase")).WillOnce(Return("{\"threatid\":[\"threatid\"]}"));
    EXPECT_CALL(*filesystemMock, writeFile("/path/persist-threatDatabase","{}"));

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase("/path");
    database.removeCorrelationID("threatid");
}

TEST_F(TestThreatDatabase, removeCorrelationIDHandlesWhenThreatIsNotInDatabase)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists("/path/persist-threatDatabase")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/path/persist-threatDatabase")).WillOnce(Return("{\"threatid\":[\"threatid\"]}"));
    EXPECT_CALL(*filesystemMock, writeFile("/path/persist-threatDatabase","{\"threatid\":[\"threatid\"]}"));

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase("/path");
    EXPECT_NO_THROW(database.removeCorrelationID("threatid2"));
}

TEST_F(TestThreatDatabase, removeCorrelationID)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists("/path/persist-threatDatabase")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/path/persist-threatDatabase")).WillOnce(Return("{\"threatid\":[\"threatid\",\"correlationID\"]}"));
    EXPECT_CALL(*filesystemMock, writeFile("/path/persist-threatDatabase","{}"));

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase("/path");
    database.removeCorrelationID( "correlationID");
}

TEST_F(TestThreatDatabase, removeThreatID)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists("/path/persist-threatDatabase")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/path/persist-threatDatabase")).WillOnce(Return("{\"threatid\":[\"threatid\",\"correlationID\"]}"));
    EXPECT_CALL(*filesystemMock, writeFile("/path/persist-threatDatabase","{}"));

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase("/path");
    database.removeThreatID("threatid");
}

TEST_F(TestThreatDatabase, removeThreatIDWhenThreatIDNotInDatabase)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists("/path/persist-threatDatabase")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/path/persist-threatDatabase")).WillOnce(Return("{}"));
    EXPECT_CALL(*filesystemMock, writeFile("/path/persist-threatDatabase","{}"));

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase("/path");
    testing::internal::CaptureStderr();
    EXPECT_NO_THROW(database.removeThreatID("threatid"));
    std::string logMessage = internal::GetCapturedStderr();
    ASSERT_THAT(logMessage, ::testing::Not(::testing::HasSubstr("Cannot remove threat id")));
}

TEST_F(TestThreatDatabase, removeThreatIDWhenThreatIDNotInDatabaseLogsWarnningWhenLogTurnedOn)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists("/path/persist-threatDatabase")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/path/persist-threatDatabase")).WillOnce(Return("{}"));
    EXPECT_CALL(*filesystemMock, writeFile("/path/persist-threatDatabase","{}"));

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase("/path");
    testing::internal::CaptureStderr();
    EXPECT_NO_THROW(database.removeThreatID("threatid",false));
    std::string logMessage = internal::GetCapturedStderr();
    ASSERT_THAT(logMessage, ::testing::HasSubstr("Cannot remove threat id"));
}

TEST_F(TestThreatDatabase, resetHealth)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists("/path/persist-threatDatabase")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/path/persist-threatDatabase")).WillOnce(Return("{\"threatid\":[\"threatid\",\"correlationID\"]}"));
    EXPECT_CALL(*filesystemMock, writeFile("/path/persist-threatDatabase","{}"));

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase("/path");
    database.resetDatabase();
}