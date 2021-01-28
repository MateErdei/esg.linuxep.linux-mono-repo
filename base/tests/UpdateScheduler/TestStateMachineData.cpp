/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "StateMachineDataBase.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <UpdateSchedulerImpl/stateMachinesModule/StateMachineData.h>
#include <UpdateSchedulerImpl/stateMachinesModule/StateMachineException.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFileSystem.h>

using namespace UpdateSchedulerImpl;
using namespace UpdateSchedulerImpl::StateData;

class StateMachineDataTest : public StateMachineDataBase
{
public:
    ~StateMachineDataTest() { }

    MockFileSystem& setupFileSystemAndGetMock()
    {
        using ::testing::Ne;
        Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, "/installroot");

        auto filesystemMock = new NiceMock<MockFileSystem>();
        ON_CALL(*filesystemMock, isDirectory(Common::ApplicationConfiguration::applicationPathManager().sophosInstall())).WillByDefault(Return(true));
        ON_CALL(*filesystemMock, isDirectory(m_primaryPath)).WillByDefault(Return(true));
        ON_CALL(*filesystemMock, isDirectory(m_distPath)).WillByDefault(Return(true));
        std::string empty;
        ON_CALL(*filesystemMock, exists(empty)).WillByDefault(Return(false));
        ON_CALL(*filesystemMock, exists(Ne(empty))).WillByDefault(Return(true));

        m_replacer.replace(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));
        return *filesystemMock;
    }
    Tests::ScopedReplaceFileSystem m_replacer;


    ::testing::AssertionResult stateMachineDataIsEquivalent(
        const char* m_expr,
        const char* n_expr,
        const StateMachineData& expected,
        const StateMachineData& resulted)
    {
        std::stringstream s;
        s << m_expr << " and " << n_expr << " failed: ";

        if (expected.getDownloadFailedSinceTime() != resulted.getDownloadFailedSinceTime())
        {
            return ::testing::AssertionFailure() << s.str() << "download failed since time  differs";
        }

        if (expected.getDownloadState() != resulted.getDownloadState())
        {
            return ::testing::AssertionFailure() << s.str() << "download state differ";
        }

        if (expected.getDownloadStateCredit() != resulted.getDownloadStateCredit())
        {
            return ::testing::AssertionFailure() << s.str() << "download state credit differ";
        }

        if (expected.getEventStateLastError() != resulted.getEventStateLastError())
        {
            return ::testing::AssertionFailure() << s.str() << "event state last error differ";
        }

        if (expected.getEventStateLastTime()!= resulted.getEventStateLastTime())
        {
            return ::testing::AssertionFailure() << s.str() << "event state last time differ";
        }

        if (expected.getInstallFailedSinceTime() != resulted.getInstallFailedSinceTime())
        {
            return ::testing::AssertionFailure() << s.str() << "install failed since time differ ";
        }

        if (expected.getInstallState() != resulted.getInstallState())
        {
            return ::testing::AssertionFailure() << s.str() << "install state differs";
        }

        if (expected.getInstallStateCredit() != resulted.getInstallStateCredit())
        {
            return ::testing::AssertionFailure() << s.str() << "install state credit differs";
        }

        if (expected.getLastGoodInstallTime() != resulted.getLastGoodInstallTime())
        {
            return ::testing::AssertionFailure() << s.str() << "last good install time differs";
        }

        return ::testing::AssertionSuccess();
    }
};

TEST_F(StateMachineDataTest, fromJsonSettingsInvalidJsonStringThrows) // NOLINT
{
    try
    {
        StateMachineData::fromJsonStateMachineData("non json string");
    }
    catch (StateMachineException& e)
    {
        EXPECT_STREQ("Failed to process json message", e.what());
    }
}

TEST_F(StateMachineDataTest, fromJsonSettingsValidButEmptyJsonStringShouldThrow) // NOLINT
{
    try
    {
        StateMachineData::fromJsonStateMachineData("{}");
    }
    catch (StateMachineException& e)
    {
        EXPECT_STREQ("Failed to process json message", e.what());
    }
}

TEST_F( // NOLINT
    StateMachineDataTest,
    fromJsonSettingsValidAndCompleteJsonStringShouldReturnValidDataObjectThatContainsExpectedEmptyData)
{
    StateMachineData expectedStateMachineData;
    expectedStateMachineData.setDownloadState("0");
    expectedStateMachineData.setDownloadFailedSinceTime("0");
    expectedStateMachineData.setDownloadStateCredit("72");
    expectedStateMachineData.setEventStateLastError("0");
    expectedStateMachineData.setEventStateLastTime("1610440731518");
    expectedStateMachineData.setInstallFailedSinceTime("0");
    expectedStateMachineData.setInstallState("0");
    expectedStateMachineData.setInstallStateCredit("3");
    expectedStateMachineData.setLastGoodInstallTime("1610465945674");

    StateMachineData actualStateMachineData = StateMachineData::fromJsonStateMachineData(createJsonString("", ""));

    EXPECT_PRED_FORMAT2(stateMachineDataIsEquivalent, actualStateMachineData, expectedStateMachineData);
}

TEST_F( // NOLINT
    StateMachineDataTest,
    fromJsonSettingsValidAndCompleteJsonStringShouldReturnValidDataObjectThatContainsExpectedValueData)
{
    StateMachineData expectedStateMachineData;
    expectedStateMachineData.setDownloadState("0");
    expectedStateMachineData.setDownloadFailedSinceTime("0");
    expectedStateMachineData.setDownloadStateCredit("72");
    expectedStateMachineData.setEventStateLastError("45");
    expectedStateMachineData.setEventStateLastTime("1610440731518");
    expectedStateMachineData.setInstallFailedSinceTime("0");
    expectedStateMachineData.setInstallState("0");
    expectedStateMachineData.setInstallStateCredit("3");
    expectedStateMachineData.setLastGoodInstallTime("1610465945674");

    std::string oldString = R"("EventStateLastError": "0")";
    std::string newString = R"("EventStateLastError": "45")";
    StateMachineData actualStateMachineData =
        StateMachineData::fromJsonStateMachineData(createJsonString(oldString, newString));

    EXPECT_PRED_FORMAT2(stateMachineDataIsEquivalent, actualStateMachineData, expectedStateMachineData);
}

TEST_F( // NOLINT
    StateMachineDataTest,
    fromJsonSettingsValidAndCompleteJsonStringShouldReturnWhenDataIncomplete)
{
    StateMachineData expectedStateMachineData;
    expectedStateMachineData.setDownloadState("1");

    std::string jsonstring = R"({"DownloadState": "1"})";

    StateMachineData actualStateMachineData = StateMachineData::fromJsonStateMachineData(jsonstring);

    EXPECT_PRED_FORMAT2(stateMachineDataIsEquivalent, actualStateMachineData, expectedStateMachineData);
}

TEST_F( // NOLINT
    StateMachineDataTest,
    fromJsonSettingsUnknownKeyShouldBeIgnored)
{
    StateMachineData expectedStateMachineData;

    std::string jsonstring = R"({"UnknownKey": "1"})";

    StateMachineData actualStateMachineData = StateMachineData::fromJsonStateMachineData(jsonstring);

    EXPECT_PRED_FORMAT2(stateMachineDataIsEquivalent, actualStateMachineData, expectedStateMachineData);
}

TEST_F( // NOLINT
    StateMachineDataTest,
    fromJsonSettingsValidAndCompleteJsonStringShouldReturnValidDataIgnoringUnknownKeys)
{
    StateMachineData expectedStateMachineData;
    expectedStateMachineData.setDownloadState("0");
    expectedStateMachineData.setDownloadFailedSinceTime("0");
    expectedStateMachineData.setDownloadStateCredit("72");
    expectedStateMachineData.setEventStateLastError("45");
    expectedStateMachineData.setEventStateLastTime("1610440731518");
    expectedStateMachineData.setInstallFailedSinceTime("0");
    expectedStateMachineData.setInstallState("0");
    expectedStateMachineData.setInstallStateCredit("3");
    expectedStateMachineData.setLastGoodInstallTime("1610465945674");

    std::string oldString = R"("EventStateLastError": "0")";
    std::string newString = R"("EventStateLastError": "45", "UnknownKey": "ignore")";
    StateMachineData actualStateMachineData =
        StateMachineData::fromJsonStateMachineData(createJsonString(oldString, newString));

    EXPECT_PRED_FORMAT2(stateMachineDataIsEquivalent, actualStateMachineData, expectedStateMachineData);
}
