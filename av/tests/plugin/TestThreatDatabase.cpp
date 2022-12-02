// Copyright 2022, Sophos Limited.  All rights reserved.

#define TEST_PUBLIC public

#include "PluginMemoryAppenderUsingTests.h"

#include "common/ApplicationPaths.h"

#include "datatypes/Print.h"

#include "Common/FileSystem/IFileSystemException.h"
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFileSystem.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"

#include <thirdparty/nlohmann-json/json.hpp>
#include <datatypes/sophos_filesystem.h>
#include <pluginimpl/ThreatDatabase.h>

namespace fs = sophos_filesystem;

namespace
{
    class TestThreatDatabase : public PluginMemoryAppenderUsingTests
    {
    protected:
        void SetUp() override
        {
            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
            appConfig.setData("SOPHOS_INSTALL", "/tmp");
            appConfig.setData("PLUGIN_INSTALL", "/tmp/av");

            Common::Telemetry::TelemetryHelper::getInstance().reset();

            m_fileSystemMock = std::make_unique<NiceMock<MockFileSystem>>();

            //For tests performing real file actions
            const ::testing::TestInfo* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
            m_testDir = fs::temp_directory_path();
            m_testDir /= test_info->test_case_name();
            m_testDir /= test_info->name();
            fs::remove_all(m_testDir);
            fs::create_directories(m_testDir);
            fs::current_path(m_testDir);
        }

        void TearDown() override
        {
            //For tests performing real file actions
            fs::current_path(fs::temp_directory_path());
            fs::remove_all(m_testDir);
        }


        std::unique_ptr<NiceMock<MockFileSystem>> m_fileSystemMock;
        fs::path m_testDir;
        const std::string m_defaultJsonString = R"({"threatid":{"correlationIds":["correlationid"],"timestamp":123}})";


        static void verifyCorruptDatabaseTelemetryPresent()
        {
            auto telemetryResult = Common::Telemetry::TelemetryHelper::getInstance().serialise();
            auto telemetry = nlohmann::json::parse(telemetryResult);

            EXPECT_TRUE(telemetry["corrupt-threat-database"]);
        }
        static void verifyCorruptDatabaseTelemetryNotPresent()
        {
            auto telemetryResult = Common::Telemetry::TelemetryHelper::getInstance().serialise();
            auto telemetry = nlohmann::json::parse(telemetryResult);

            EXPECT_EQ(telemetry["corrupt-threat-database"], nullptr);
        }
    };
}

TEST_F(TestThreatDatabase, initDatabaseWorksWhenNoFile)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_fileSystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(false));
    EXPECT_CALL(*m_fileSystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),"{}"));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_fileSystemMock) };

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());

    EXPECT_TRUE(waitForLog("Initialised Threat Database"));
    verifyCorruptDatabaseTelemetryNotPresent();
}

TEST_F(TestThreatDatabase, initDatabaseHandlesEmptyStringInFile)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_fileSystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_fileSystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(""));
    EXPECT_CALL(*m_fileSystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),"{}"));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_fileSystemMock) };

    EXPECT_NO_THROW(Plugin::ThreatDatabase{Plugin::getPluginVarDirPath()});

    EXPECT_TRUE(waitForLog("Resetting ThreatDatabase as we failed to parse ThreatDatabase on disk with error"));
    verifyCorruptDatabaseTelemetryPresent();
}

TEST_F(TestThreatDatabase, initDatabaseHandlesEmptyDatabaseInFile)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_fileSystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return("{}"));
    EXPECT_CALL(*m_fileSystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),"{}"));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_fileSystemMock) };

    EXPECT_NO_THROW(Plugin::ThreatDatabase{Plugin::getPluginVarDirPath()});

    EXPECT_FALSE(appenderContains("into threat database as the parsing failed with error"));
    verifyCorruptDatabaseTelemetryNotPresent();
}

TEST_F(TestThreatDatabase, initDatabaseHandlesMalformedJsonStringInFile)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_fileSystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_fileSystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return("{:stuf}"));
    EXPECT_CALL(*m_fileSystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),"{}"));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_fileSystemMock) };

    EXPECT_NO_THROW(Plugin::ThreatDatabase{Plugin::getPluginVarDirPath()});

    EXPECT_FALSE(appenderContains("into threat database as the parsing failed with error"));
    verifyCorruptDatabaseTelemetryPresent();
}


TEST_F(TestThreatDatabase, DatabaseLoadsAndSavesCorrectly)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_fileSystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(m_defaultJsonString));
    EXPECT_CALL(*m_fileSystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),m_defaultJsonString));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_fileSystemMock) };

    EXPECT_NO_THROW(Plugin::ThreatDatabase{Plugin::getPluginVarDirPath()});

    EXPECT_FALSE(appenderContains("into threat database as the parsing failed with error"));
    verifyCorruptDatabaseTelemetryNotPresent();
}

TEST_F(TestThreatDatabase, isDatabaseEmptyReturnsTrueOnEmptyDatabase)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_fileSystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return("{}"));
    EXPECT_CALL(*m_fileSystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),"{}"));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_fileSystemMock) };

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());
    EXPECT_TRUE(waitForLog("Initialised Threat Database"));

    EXPECT_TRUE(database.isDatabaseEmpty());

    EXPECT_FALSE(appenderContains("into threat database as the parsing failed with error"));
    verifyCorruptDatabaseTelemetryNotPresent();
}

TEST_F(TestThreatDatabase, isDatabaseEmptyReturnsFalseOnNonEmptyDatabase)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_fileSystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return("{}"));
    EXPECT_CALL(*m_fileSystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),_));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_fileSystemMock) };

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());
    EXPECT_TRUE(waitForLog("Initialised Threat Database"));

    database.addThreat("threat","correlationid");
    EXPECT_FALSE(database.isDatabaseEmpty());

    EXPECT_FALSE(appenderContains("into threat database as the parsing failed with error"));
    verifyCorruptDatabaseTelemetryNotPresent();
}

TEST_F(TestThreatDatabase, addThreatToDatabase)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string addedThreat = "threatID";

    EXPECT_CALL(*m_fileSystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(m_defaultJsonString));
    EXPECT_CALL(*m_fileSystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),_));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_fileSystemMock) };

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());
    EXPECT_TRUE(waitForLog("Initialised Threat Database"));

    database.addThreat(addedThreat,"correlationid2");

    auto databaseContents = database.m_database.lock();
    EXPECT_EQ(databaseContents->size(), 2);
    EXPECT_TRUE(databaseContents->find(addedThreat) != databaseContents->end());

    EXPECT_FALSE(appenderContains("into threat database as the parsing failed with error"));
    verifyCorruptDatabaseTelemetryNotPresent();
}

TEST_F(TestThreatDatabase, addCorrelationIDToDatabase)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string threatid = "threatid";
    const std::string correlationid = "correlationid";

    EXPECT_CALL(*m_fileSystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(m_defaultJsonString));
    EXPECT_CALL(*m_fileSystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),_));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_fileSystemMock) };

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());
    EXPECT_TRUE(waitForLog("Initialised Threat Database"));

    database.addThreat(threatid,correlationid);

    auto databaseContents = database.m_database.lock();
    ASSERT_EQ(databaseContents->size(), 1);
    auto threatidItems = databaseContents->find(threatid);
    ASSERT_TRUE(threatidItems != databaseContents->end());
    auto correlationThreatItem =
        std::find(threatidItems->second.correlationIds.cbegin(), threatidItems->second.correlationIds.cend(), correlationid);
    EXPECT_TRUE(correlationThreatItem != threatidItems->second.correlationIds.cend());

    EXPECT_FALSE(appenderContains("into threat database as the parsing failed with error"));
    verifyCorruptDatabaseTelemetryNotPresent();
}

TEST_F(TestThreatDatabase, DatabaseLoadsAndSavesHandlesUnexpectedStructureNewCorrelationID)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_fileSystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath()))
        .WillOnce(Return(R"({"threatid":{"correlationIds":1000,"timestamp":123}})"));
    EXPECT_CALL(*m_fileSystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),_));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_fileSystemMock) };

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());

    EXPECT_TRUE(waitForLog("type must be string, but is number"));

    auto databaseContents = database.m_database.lock();
    EXPECT_EQ(databaseContents->size(), 0);

    verifyCorruptDatabaseTelemetryNotPresent();
}

TEST_F(TestThreatDatabase, DatabaseLoadsAndHandlesUnexpectedStructureNewThreatID)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_fileSystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_fileSystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath()))
        .WillOnce(Return(R"({threatid:{"correlationIds":"threatid","timestamp":123}})"));
    EXPECT_CALL(*m_fileSystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),_));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_fileSystemMock) };

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());

    EXPECT_TRUE(waitForLog("Resetting ThreatDatabase as we failed to parse ThreatDatabase on disk with error"));

    auto databaseContents = database.m_database.lock();
    EXPECT_EQ(databaseContents->size(), 0);

    verifyCorruptDatabaseTelemetryPresent();
}

TEST_F(TestThreatDatabase, removeCorrelationIDRemovesThreat)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_fileSystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(m_defaultJsonString));
    EXPECT_CALL(*m_fileSystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),"{}"));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_fileSystemMock) };

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());
    EXPECT_TRUE(waitForLog("Initialised Threat Database"));

    database.removeCorrelationID("correlationid");

    EXPECT_TRUE(waitForLog("Removed threat from database"));
    EXPECT_FALSE(appenderContains("into threat database as the parsing failed with error"));

    auto databaseContents = database.m_database.lock();
    EXPECT_EQ(databaseContents->size(), 0);

    verifyCorruptDatabaseTelemetryNotPresent();
}

TEST_F(TestThreatDatabase, removeCorrelationIDHandlesWhenThreatIsNotInDatabase)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_fileSystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(m_defaultJsonString));
    EXPECT_CALL(*m_fileSystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),_));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_fileSystemMock) };

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());
    EXPECT_TRUE(waitForLog("Initialised Threat Database"));

    EXPECT_NO_THROW(database.removeCorrelationID("threatid2"));
    EXPECT_TRUE(waitForLog("Cannot remove correlation idthreatid2 from database as it cannot be found"));

    auto databaseContents = database.m_database.lock();
    ASSERT_EQ(databaseContents->size(), 1);
    auto threatidItems = databaseContents->begin();
    EXPECT_EQ(threatidItems->first, "threatid");
    EXPECT_EQ(threatidItems->second.correlationIds.size(), 1);
    EXPECT_EQ(threatidItems->second.correlationIds.front(), "correlationid");

    verifyCorruptDatabaseTelemetryNotPresent();
}

TEST_F(TestThreatDatabase, removeThreatIDHandlesWhenThreatIsNotInDatabase)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_fileSystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(m_defaultJsonString));
    EXPECT_CALL(*m_fileSystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),_));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_fileSystemMock) };

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());
    EXPECT_TRUE(waitForLog("Initialised Threat Database"));

    EXPECT_NO_THROW(database.removeThreatID("threatid2",false));
    EXPECT_TRUE(waitForLog("Cannot remove threat idthreatid2 from database as it cannot be found"));

    auto databaseContents = database.m_database.lock();
    ASSERT_EQ(databaseContents->size(), 1);
    auto threatidItems = databaseContents->begin();
    EXPECT_EQ(threatidItems->first, "threatid");
    EXPECT_EQ(threatidItems->second.correlationIds.size(), 1);
    EXPECT_EQ(threatidItems->second.correlationIds.front(), "correlationid");

    verifyCorruptDatabaseTelemetryNotPresent();
}

TEST_F(TestThreatDatabase, removeThreatID)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::string threatIDToRemove = "threatid";

    EXPECT_CALL(*m_fileSystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(m_defaultJsonString));
    EXPECT_CALL(*m_fileSystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),"{}"));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_fileSystemMock) };

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());
    EXPECT_TRUE(waitForLog("Initialised Threat Database"));

    database.removeThreatID(threatIDToRemove);

    std::stringstream expectString;
    expectString << "Removed threat id " << threatIDToRemove << " from database";
    EXPECT_TRUE(waitForLog(expectString.str()));
    EXPECT_FALSE(appenderContains("into threat database as the parsing failed with error"));
    verifyCorruptDatabaseTelemetryNotPresent();
}

TEST_F(TestThreatDatabase, removeThreatIDWhenThreatIDNotInDatabase)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_fileSystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return("{}"));
    EXPECT_CALL(*m_fileSystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),"{}"));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_fileSystemMock) };

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());
    EXPECT_NO_THROW(database.removeThreatID("threatid", false));

    EXPECT_TRUE(waitForLog("Cannot remove threat id"));
    verifyCorruptDatabaseTelemetryNotPresent();
}

TEST_F(TestThreatDatabase, removeThreatIDWhenThreatIDNotInDatabaseLogsWarnningWhenLogTurnedOn)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_fileSystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return("{}"));
    EXPECT_CALL(*m_fileSystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),"{}"));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_fileSystemMock) };

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());
    EXPECT_TRUE(waitForLog("Initialised Threat Database"));

    EXPECT_NO_THROW(database.removeThreatID("threatid",false));
    EXPECT_TRUE(waitForLog("Cannot remove threat id"));
    verifyCorruptDatabaseTelemetryNotPresent();
}

TEST_F(TestThreatDatabase, resetHealth)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_fileSystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(m_defaultJsonString));
    EXPECT_CALL(*m_fileSystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),"{}")).Times(2);

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_fileSystemMock) };

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());
    database.resetDatabase();

    EXPECT_FALSE(appenderContains("into threat database as the parsing failed with error"));
    verifyCorruptDatabaseTelemetryNotPresent();
}

TEST_F(TestThreatDatabase, resetHealthHandlesFileError)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_fileSystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(m_defaultJsonString));
    EXPECT_CALL(*m_fileSystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),"{}"))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("exception"))).WillOnce(Return());

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_fileSystemMock) };

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());
    EXPECT_NO_THROW(database.resetDatabase());

    EXPECT_TRUE(waitForLog("Cannot reset ThreatDatabase on disk with error: "));
    EXPECT_FALSE(appenderContains("into threat database as the parsing failed with error"));
    verifyCorruptDatabaseTelemetryNotPresent();
}

TEST_F(TestThreatDatabase, handlesInvalidFormatTime)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_fileSystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath()))
        .WillOnce(Return(R"({"threatid":{"correlationIds":"correlationId","timestamp":"123"}})"));
    EXPECT_CALL(*m_fileSystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),_));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_fileSystemMock) };

    Plugin::ThreatDatabase threatDatabase = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());

    EXPECT_TRUE(waitForLog("type must be number, but is string"));
    auto database = threatDatabase.m_database.lock();
    ASSERT_EQ(database->size(), 1);
    const long timeStamp = std::chrono::time_point_cast<std::chrono::seconds>(database->begin()->second.lastDetection).time_since_epoch().count();
    EXPECT_EQ(timeStamp, 0);

    verifyCorruptDatabaseTelemetryNotPresent();
}

TEST_F(TestThreatDatabase, handlesNegativeFormatTime)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_fileSystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath()))
        .WillOnce(Return(R"({"threatid":{"correlationIds":"correlationId","timestamp":-1}})"));
    EXPECT_CALL(*m_fileSystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),_));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_fileSystemMock) };

    Plugin::ThreatDatabase threatDatabase = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());

    EXPECT_TRUE(waitForLog("Initialised Threat Database"));
    auto database = threatDatabase.m_database.lock();
    ASSERT_EQ(database->size(), 1);
    const long timeStamp = std::chrono::time_point_cast<std::chrono::seconds>(database->begin()->second.lastDetection).time_since_epoch().count();
    EXPECT_EQ(timeStamp, -1);

    verifyCorruptDatabaseTelemetryNotPresent();
}

TEST_F(TestThreatDatabase, handlesOutOfBoundsTimeValue)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_fileSystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath()))
        .WillOnce(Return(R"({"threatid":{"correlationIds":"correlationId","timestamp":18446744073709551620}})"));
    EXPECT_CALL(*m_fileSystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),_));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_fileSystemMock) };

    Plugin::ThreatDatabase threatDatabase = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());

    EXPECT_TRUE(waitForLog("Initialised Threat Database"));
    auto database = threatDatabase.m_database.lock();
    ASSERT_EQ(database->size(), 1);
    const long timeStamp = std::chrono::time_point_cast<std::chrono::seconds>(database->begin()->second.lastDetection).time_since_epoch().count();
    EXPECT_EQ(timeStamp, 0);

    verifyCorruptDatabaseTelemetryNotPresent();
}

TEST_F(TestThreatDatabase, writesValidJsonOnDestruct)
{
    const std::string expectLocation = m_testDir.string() + "/persist-threatDatabase";

    auto threatDatabase = std::make_unique<Plugin::ThreatDatabase>(m_testDir.string());

    threatDatabase->addThreat("threat1","correlation1");
    threatDatabase.reset(nullptr);

    ASSERT_TRUE(fs::exists(expectLocation));
    PRINT(expectLocation);
    nlohmann::json j;
    try
    {
        auto fileSystem = Common::FileSystem::fileSystem();
        auto configContents = fileSystem->readFile(expectLocation);
        j = nlohmann::json::parse(configContents);
    }
    catch (nlohmann::json::exception& ex)
    {
        PRINT(ex.what());
        FAIL() << "Failed to parse json that was written on destruct";
    }
}

TEST_F(TestThreatDatabase, readsDatabaseOnConstruction)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string location = m_testDir.string() + "/persist-threatDatabase";

    auto fileSystem = Common::FileSystem::fileSystem();
    fileSystem->writeFile(location, m_defaultJsonString);
    ASSERT_TRUE(fs::exists(location));

    auto threatDatabase = Plugin::ThreatDatabase( m_testDir.string());
    EXPECT_TRUE(waitForLog("Initialised Threat Database"));
    EXPECT_FALSE(appenderContains("Resetting ThreatDatabase as we failed to parse ThreatDatabase on disk with error: "));

    auto threatContents = threatDatabase.m_databaseInString.getValue();
    EXPECT_EQ(m_defaultJsonString, threatContents);
}

TEST_F(TestThreatDatabase, dataWrittenToDiskMatchesWhenRead)
{
    auto threatDatabase = Plugin::ThreatDatabase(m_testDir.string() + "/persist-threatDatabase");

    const std::string threat1 = "threat1";
    const std::string threat2 = "threat2";
    const std::string correlation1 = "correlation1";
    const std::string correlation2 = "correlation2";
    const long timeStamp = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now()).time_since_epoch().count();

    threatDatabase.addThreat(threat1,correlation1);
    threatDatabase.addThreat(threat1,correlation2);
    threatDatabase.addThreat(threat2,correlation1);

    threatDatabase.convertDatabaseToString();
    threatDatabase.convertStringToDatabase();

    auto res = threatDatabase.m_database.lock();

    //Check total map
    ASSERT_EQ(res->size(), 2);

    //Check threat1 correlations
    EXPECT_EQ(res->at(threat1).correlationIds.size(), 2);
    EXPECT_EQ(res->at(threat1).correlationIds.front(), correlation1);
    EXPECT_EQ(res->at(threat1).correlationIds.back(), correlation2);

    const long threat1Time = std::chrono::time_point_cast<std::chrono::seconds>(res->at(threat1).lastDetection).time_since_epoch().count();
    EXPECT_TRUE((threat1Time - timeStamp) <= 1); //Test should take a second or less to complete

    //Check threat2 correlations
    EXPECT_EQ(res->at(threat2).correlationIds.size(), 1);
    EXPECT_EQ(res->at(threat2).correlationIds.front(), correlation1);
    const long threat2Time = std::chrono::time_point_cast<std::chrono::seconds>(res->at(threat2).lastDetection).time_since_epoch().count();
    EXPECT_TRUE((threat2Time - timeStamp) <= 1); //Test should take a second or less to complete
}


TEST_F(TestThreatDatabase, threatDatabaseReturnsTrueIfThreatAddedUnderTimeout)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string threatId = "threatid";

    EXPECT_CALL(*m_fileSystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return("{}" ));
    EXPECT_CALL(*m_fileSystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),_));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_fileSystemMock) };

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());
    EXPECT_TRUE(waitForLog("Initialised Threat Database"));

    database.addThreat(threatId, "correlationid");
    EXPECT_TRUE(database.isThreatInDatabaseWithinTime(threatId, std::chrono::seconds{60}));
}

TEST_F(TestThreatDatabase, threatDatabaseReturnsFalseIfThreatAddedOverTimeout)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_fileSystemMock, exists(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, readFile(Plugin::getPersistThreatDatabaseFilePath())).WillOnce(Return(m_defaultJsonString));
    EXPECT_CALL(*m_fileSystemMock, writeFile(Plugin::getPersistThreatDatabaseFilePath(),_));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_fileSystemMock) };

    Plugin::ThreatDatabase database = Plugin::ThreatDatabase(Plugin::getPluginVarDirPath());
    EXPECT_TRUE(waitForLog("Initialised Threat Database"));

    EXPECT_FALSE(database.isThreatInDatabaseWithinTime("threatid", std::chrono::seconds{60}));
}