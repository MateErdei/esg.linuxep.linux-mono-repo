// Copyright 2022, Sophos Limited.  All rights reserved.

#include "tests/common/LogInitializedTests.h"

#include "Common/FileSystem/IFileSystemException.h"
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFileSystem.h"
#include "common/ApplicationPaths.h"

#include <datatypes/sophos_filesystem.h>
#include <pluginimpl/ThreatDatabase.h>

namespace fs = sophos_filesystem;

namespace
{
    class TestThreatDatabase : public LogInitializedTests
    {
        void SetUp() override
        {
            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
            appConfig.setData("SOPHOS_INSTALL", "/tmp");
            appConfig.setData("PLUGIN_INSTALL", "/tmp/av");
        }
    };
}

TEST_F(TestThreatDatabase, initDatabaseWorksWhenNoFile)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),"{}"));

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());
}

TEST_F(TestThreatDatabase, initDatabaseHandlesEmptyStringInFile)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(""));
    EXPECT_CALL(*filesystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),"{}"));

    EXPECT_NO_THROW(Plugin::ThreatDatabase{Plugin::getPluginVarDirPath()});
}

TEST_F(TestThreatDatabase, initDatabaseHandlesEmptyDatabaseInFile)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return("{}"));
    EXPECT_CALL(*filesystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),"{}"));

    EXPECT_NO_THROW(Plugin::ThreatDatabase{Plugin::getPluginVarDirPath()});
}

TEST_F(TestThreatDatabase, initDatabaseHandlesMalformedJsonStringInFile)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return("{:stuf}"));
    EXPECT_CALL(*filesystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),"{}"));

    EXPECT_NO_THROW(Plugin::ThreatDatabase{Plugin::getPluginVarDirPath()});
}

TEST_F(TestThreatDatabase, initDatabaseHandlesPermissionError)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Throw(Common::FileSystem::IFileSystemException("exception")));
    EXPECT_CALL(*filesystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),"{}"));

    EXPECT_NO_THROW(Plugin::ThreatDatabase{Plugin::getPluginVarDirPath()});
}

TEST_F(TestThreatDatabase, DatabaseLoadsAndSavesCorrectly)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(R"({"threatid":["threatid"]})"));
    EXPECT_CALL(*filesystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),"{\"threatid\":[\"threatid\"]}"));

    EXPECT_NO_THROW(Plugin::ThreatDatabase{Plugin::getPluginVarDirPath()});
}

TEST_F(TestThreatDatabase, isDatabaseEmptyReturnsTrueOnEmptyDatabase)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return("{}"));
    EXPECT_CALL(*filesystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),"{}"));

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());
    EXPECT_TRUE(database.isDatabaseEmpty());
}

TEST_F(TestThreatDatabase, isDatabaseEmptyReturnsFalseOnNonEmptyDatabase)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return("{}"));
    EXPECT_CALL(*filesystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),"{\"threat\":[\"threat\"]}"));

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());
    database.addThreat("threat","threat");
    EXPECT_FALSE(database.isDatabaseEmpty());
}

TEST_F(TestThreatDatabase, addThreatToDatabase)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(R"({"threatid":["threatid"]})"));
    EXPECT_CALL(*filesystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),"{\"threatID\":[\"correlationid2\"],\"threatid\":[\"threatid\"]}"));

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());
    database.addThreat("threatID","correlationid2");
}

TEST_F(TestThreatDatabase, addCorrelationIDToDatabase)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(R"({"threatid":["threatid"]})"));
    EXPECT_CALL(*filesystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),"{\"threatid\":[\"threatid\",\"correlationid\"]}"));

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());
    database.addThreat("threatid","correlationid");
}

TEST_F(TestThreatDatabase, DatabaseLoadsAndSavesHandlesUnexpectedStructureNewCorrelationID)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return("{\"threatid\":1000}"));
    EXPECT_CALL(*filesystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),"{\"threatid\":[\"correlationid2\"]}"));

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());
    database.addThreat("threatid","correlationid2");
}

TEST_F(TestThreatDatabase, DatabaseLoadsAndSavesHandlesUnexpectedStructureNewThreatID)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return("{\"threatID1\":1000}"));
    EXPECT_CALL(*filesystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),"{\"threatID2\":[\"correlationid2\"]}"));

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());
    database.addThreat("threatID2","correlationid2");
}

TEST_F(TestThreatDatabase, removeCorrelationIDRemovesThreatWhenNoMoreCorrelationIDsForThreatID)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(R"({"threatid":["threatid"]})"));
    EXPECT_CALL(*filesystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),"{}"));

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());
    database.removeCorrelationID("threatid");
}

TEST_F(TestThreatDatabase, removeCorrelationIDHandlesWhenThreatIsNotInDatabase)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(R"({"threatid":["threatid"]})"));
    EXPECT_CALL(*filesystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),"{\"threatid\":[\"threatid\"]}"));

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());
    EXPECT_NO_THROW(database.removeCorrelationID("threatid2"));
}

TEST_F(TestThreatDatabase, removeThreatIDHandlesWhenThreatIsNotInDatabase)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(R"({"threatid":["threatid"]})"));
    EXPECT_CALL(*filesystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),"{\"threatid\":[\"threatid\"]}"));

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());
    EXPECT_NO_THROW(database.removeThreatID("threatid2","threatid"));
}

TEST_F(TestThreatDatabase, removeCorrelationID)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(R"({"threatid":["threatid","correlationID"]})"));
    EXPECT_CALL(*filesystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),"{}"));

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());
    database.removeCorrelationID( "correlationID");
}

TEST_F(TestThreatDatabase, removeThreatID)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(R"({"threatid":["threatid","correlationID"]})"));
    EXPECT_CALL(*filesystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),"{}"));

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());
    database.removeThreatID("threatid");
}

TEST_F(TestThreatDatabase, removeThreatIDWhenThreatIDNotInDatabase)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return("{}"));
    EXPECT_CALL(*filesystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),"{}"));

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());
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

    EXPECT_CALL(*filesystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return("{}"));
    EXPECT_CALL(*filesystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),"{}"));

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());
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

    EXPECT_CALL(*filesystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(R"({"threatid":["threatid","correlationID"]})"));
    EXPECT_CALL(*filesystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),"{}")).Times(2);

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());
    database.resetDatabase();
}

TEST_F(TestThreatDatabase, resetHealthHandlesFileError)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(R"({"threatid":["threatid","correlationID"]})"));
    EXPECT_CALL(*filesystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),"{}")).WillOnce(Throw(Common::FileSystem::IFileSystemException("exception"))).WillOnce(Return());

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());
    EXPECT_NO_THROW(database.resetDatabase());
}