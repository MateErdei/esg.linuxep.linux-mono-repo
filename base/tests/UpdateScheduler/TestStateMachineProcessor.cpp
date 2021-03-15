/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "StateMachineDataBase.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/FileSystem/IFileTooLargeException.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <UpdateSchedulerImpl/stateMachinesModule/StateMachineProcessor.h>
#include <UpdateSchedulerImpl/configModule/UpdateEvent.h>
#include <UpdateSchedulerImpl/stateMachinesModule/StateMachineData.h>
#include <UpdateSchedulerImpl/stateMachinesModule/StateMachineException.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFileSystem.h>

using namespace UpdateSchedulerImpl;
using namespace UpdateSchedulerImpl::StateData;

class StateMachineProcessorTest : public StateMachineDataBase
{
public:
    ~StateMachineProcessorTest() { }

    MockFileSystem& setupFileSystemAndGetMock()
    {
        return setupFileSystemAndGetMock(true);
    }
    MockFileSystem& setupFileSystemAndGetMock(bool nonEmptyExistsFilePathDefaultValue)
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
        ON_CALL(*filesystemMock, exists(Ne(empty))).WillByDefault(Return(nonEmptyExistsFilePathDefaultValue));

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
            return ::testing::AssertionFailure() << s.str() << "download failed since time  differs" << expected.getDownloadFailedSinceTime() << ":" << resulted.getDownloadFailedSinceTime();
        }

        if (expected.getDownloadState() != resulted.getDownloadState())
        {
            return ::testing::AssertionFailure() << s.str() << "download state differ" << expected.getDownloadState() << ":" << resulted.getDownloadState();
        }

        if (expected.getDownloadStateCredit() != resulted.getDownloadStateCredit())
        {
            return ::testing::AssertionFailure() << s.str() << "download state credit differ" << expected.getDownloadStateCredit() << ":" << resulted.getDownloadStateCredit();
        }

        if (expected.getEventStateLastError() != resulted.getEventStateLastError())
        {
            return ::testing::AssertionFailure() << s.str() << "event state last error differ" << expected.getEventStateLastError() << ":" << resulted.getEventStateLastError();
        }

        if (expected.getEventStateLastTime()!= resulted.getEventStateLastTime())
        {
            return ::testing::AssertionFailure() << s.str() << "event state last time differ" << expected.getEventStateLastTime() << ":" << resulted.getEventStateLastTime();
        }

        if (expected.getInstallFailedSinceTime() != resulted.getInstallFailedSinceTime())
        {
            return ::testing::AssertionFailure() << s.str() << "install failed since time differ " << expected.getInstallFailedSinceTime() << ":" << resulted.getInstallFailedSinceTime();
        }

        if (expected.getInstallState() != resulted.getInstallState())
        {
            return ::testing::AssertionFailure() << s.str() << "install state differs" << expected.getInstallState() << ":" << resulted.getInstallState();
        }

        if (expected.getInstallStateCredit() != resulted.getInstallStateCredit())
        {
            return ::testing::AssertionFailure() << s.str() << "install state credit differs" << expected.getInstallStateCredit() << ":" << resulted.getInstallStateCredit();
        }

        if (expected.getLastGoodInstallTime() != resulted.getLastGoodInstallTime())
        {
            return ::testing::AssertionFailure() << s.str() << "last good install time differs" << expected.getLastGoodInstallTime() << ":" << resulted.getLastGoodInstallTime();
        }

        return ::testing::AssertionSuccess();
    }
};

class StateMachineProcessorTestWithCredit : public StateMachineProcessorTest, public ::testing::WithParamInterface<std::tuple<std::string, long, long, int>>
{
};

class StateMachineProcessorTestWithException : public StateMachineProcessorTest, public ::testing::WithParamInterface<std::tuple<std::string, std::string>>
{
};

TEST_F(StateMachineProcessorTest, StateMachinesCorrectlyUpdatedWhenUpdateResultIsSuccess) // NOLINT
{
    std::string rawJsonStateMachineData = createJsonString("", "");

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData));

    stateMachinesModule::StateMachineProcessor stateMachineProcessor("1610465945");

    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::SUCCESS);

    StateMachineData expectedStateMachineData = StateMachineData::fromJsonStateMachineData(createJsonString("", ""));
    EXPECT_NE( expectedStateMachineData.getEventStateLastTime(),stateMachineProcessor.getStateMachineData().getEventStateLastTime());
    expectedStateMachineData.setEventStateLastTime(stateMachineProcessor.getStateMachineData().getEventStateLastTime());

    EXPECT_PRED_FORMAT2(stateMachineDataIsEquivalent, expectedStateMachineData, stateMachineProcessor.getStateMachineData());

}

TEST_F(StateMachineProcessorTest, StateMachinesCorrectlyUpdatedWhenUpdateResultIsInstallFailed) // NOLINT
{
    std::string rawJsonStateMachineData = createJsonString( R"("InstallStateCredit": "3")", R"("InstallStateCredit": "1")");

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData));

    stateMachinesModule::StateMachineProcessor stateMachineProcessor("1610465945");

    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::INSTALLFAILED);

    StateMachineData expectedStateMachineData = StateMachineData::fromJsonStateMachineData(createJsonString("", ""));
    expectedStateMachineData.setInstallStateCredit("0");
    expectedStateMachineData.setInstallState("1");
    expectedStateMachineData.setEventStateLastError(std::to_string(configModule::EventMessageNumber::INSTALLFAILED));

    EXPECT_NE(expectedStateMachineData.getInstallFailedSinceTime(), stateMachineProcessor.getStateMachineData().getInstallFailedSinceTime());
    EXPECT_NE(expectedStateMachineData.getEventStateLastTime(), stateMachineProcessor.getStateMachineData().getEventStateLastTime());
    // update expected time stamps to match actual for the times that are expected to change.
    expectedStateMachineData.setInstallFailedSinceTime(stateMachineProcessor.getStateMachineData().getInstallFailedSinceTime());
    expectedStateMachineData.setEventStateLastTime(stateMachineProcessor.getStateMachineData().getEventStateLastTime());

    EXPECT_PRED_FORMAT2(stateMachineDataIsEquivalent, expectedStateMachineData, stateMachineProcessor.getStateMachineData());

}

TEST_F(StateMachineProcessorTest, StateMachinesCorrectlyUpdatedWhenFirstUpdateResultIsInstallFailed) // NOLINT
{
    std::string rawJsonStateMachineData = createJsonString( "", "");

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData));

    stateMachinesModule::StateMachineProcessor stateMachineProcessor("1610465945");

    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::INSTALLFAILED);

    StateMachineData expectedStateMachineData = StateMachineData::fromJsonStateMachineData(createJsonString("", ""));
    expectedStateMachineData.setInstallStateCredit("2");
    expectedStateMachineData.setInstallState("0");

    EXPECT_PRED_FORMAT2(stateMachineDataIsEquivalent, expectedStateMachineData, stateMachineProcessor.getStateMachineData());

}


TEST_F(StateMachineProcessorTest, StateMachinesCorrectlyUpdatedWhenUpdateResultIsInstallFailedThenSucceeded) // NOLINT
{
    // Test when next update fails
    std::string rawJsonStateMachineData = createJsonString(R"("InstallStateCredit": "3")", R"("InstallStateCredit": "1")");

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData)).RetiresOnSaturation();

    stateMachinesModule::StateMachineProcessor stateMachineProcessor1("1610465945");

    stateMachineProcessor1.updateStateMachines(configModule::EventMessageNumber::INSTALLFAILED);

    StateMachineData expectedStateMachineData = StateMachineData::fromJsonStateMachineData(createJsonString("", ""));
    expectedStateMachineData.setInstallStateCredit("0");
    expectedStateMachineData.setInstallState("1");

    EXPECT_NE(expectedStateMachineData.getInstallFailedSinceTime(),
        stateMachineProcessor1.getStateMachineData().getInstallFailedSinceTime());
    EXPECT_NE(expectedStateMachineData.getEventStateLastTime(),
              stateMachineProcessor1.getStateMachineData().getEventStateLastTime());
    // update expected time stamps to match actual for the times that are expected to change.
    expectedStateMachineData.setInstallFailedSinceTime(
        stateMachineProcessor1.getStateMachineData().getInstallFailedSinceTime());
    expectedStateMachineData.setEventStateLastTime(stateMachineProcessor1.getStateMachineData().getEventStateLastTime());
    expectedStateMachineData.setEventStateLastError(std::to_string(configModule::EventMessageNumber::INSTALLFAILED));

    EXPECT_PRED_FORMAT2(stateMachineDataIsEquivalent, expectedStateMachineData, stateMachineProcessor1.getStateMachineData());

    // Test when next update is successful

    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(expectedStateMachineData.toJsonStateMachineData(expectedStateMachineData)));
    stateMachinesModule::StateMachineProcessor stateMachineProcessor2("1610465945"); //File will be loaded
    stateMachineProcessor2.updateStateMachines(configModule::EventMessageNumber::SUCCESS);

    EXPECT_NE(expectedStateMachineData.getInstallFailedSinceTime(),stateMachineProcessor2.getStateMachineData().getInstallFailedSinceTime());

    // update expected time stamps to match actual for the times that are expected to change.
    expectedStateMachineData.setInstallFailedSinceTime(stateMachineProcessor2.getStateMachineData().getInstallFailedSinceTime());
    expectedStateMachineData.setInstallStateCredit("3");
    expectedStateMachineData.setInstallState("0");
    expectedStateMachineData.setEventStateLastError(std::to_string(configModule::EventMessageNumber::SUCCESS));

    EXPECT_PRED_FORMAT2(stateMachineDataIsEquivalent, expectedStateMachineData, stateMachineProcessor2.getStateMachineData());

}




TEST_F(StateMachineProcessorTest, StateMachinesCorrectlyUpdatedWhenUpdateResultIsUpdateSourceMissing) // NOLINT
{
    std::string rawJsonStateMachineData = createJsonString(R"("InstallStateCredit": "3")", R"("InstallStateCredit": "1")");
    rawJsonStateMachineData = updateJsonString(rawJsonStateMachineData, R"("DownloadStateCredit": "72")", R"("DownloadStateCredit": "18")");

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData));

    stateMachinesModule::StateMachineProcessor stateMachineProcessor("1610465945");

    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::UPDATESOURCEMISSING);

    StateMachineData expectedStateMachineData = StateMachineData::fromJsonStateMachineData(createJsonString("", ""));

    expectedStateMachineData.setDownloadStateCredit("0");
    expectedStateMachineData.setInstallStateCredit("0");
    expectedStateMachineData.setDownloadState("1");
    expectedStateMachineData.setInstallState("1");

    EXPECT_NE(expectedStateMachineData.getDownloadFailedSinceTime(), stateMachineProcessor.getStateMachineData().getDownloadFailedSinceTime());
    EXPECT_NE(expectedStateMachineData.getInstallFailedSinceTime(), stateMachineProcessor.getStateMachineData().getInstallFailedSinceTime());
    EXPECT_NE(expectedStateMachineData.getEventStateLastTime(), stateMachineProcessor.getStateMachineData().getEventStateLastTime());
    // update expected time stamps to match actual for the times that are expected to change.
    expectedStateMachineData.setDownloadFailedSinceTime(stateMachineProcessor.getStateMachineData().getDownloadFailedSinceTime());
    expectedStateMachineData.setInstallFailedSinceTime(stateMachineProcessor.getStateMachineData().getInstallFailedSinceTime());
    expectedStateMachineData.setEventStateLastTime(stateMachineProcessor.getStateMachineData().getEventStateLastTime());
    expectedStateMachineData.setEventStateLastError(std::to_string(configModule::EventMessageNumber::UPDATESOURCEMISSING));

    EXPECT_PRED_FORMAT2(stateMachineDataIsEquivalent, expectedStateMachineData, stateMachineProcessor.getStateMachineData());
}

TEST_F(StateMachineProcessorTest, StateMachinesCorrectlyUpdatedWhenUpdateResultIstSinglePackageMissing) // NOLINT
{
    std::string rawJsonStateMachineData = createJsonString(R"("InstallStateCredit": "3")", R"("InstallStateCredit": "1")");
    rawJsonStateMachineData = updateJsonString(rawJsonStateMachineData, R"("DownloadStateCredit": "72")", R"("DownloadStateCredit": "18")");

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData));

    stateMachinesModule::StateMachineProcessor stateMachineProcessor("1610465945");

    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::SINGLEPACKAGEMISSING);

    StateMachineData expectedStateMachineData = StateMachineData::fromJsonStateMachineData(createJsonString("", ""));

    expectedStateMachineData.setDownloadStateCredit("0");
    expectedStateMachineData.setInstallStateCredit("1");
    expectedStateMachineData.setDownloadState("1");
    expectedStateMachineData.setInstallState("0");
    expectedStateMachineData.setEventStateLastError(std::to_string(configModule::EventMessageNumber::SINGLEPACKAGEMISSING));

    EXPECT_NE(expectedStateMachineData.getDownloadFailedSinceTime(), stateMachineProcessor.getStateMachineData().getDownloadFailedSinceTime());
    EXPECT_NE(expectedStateMachineData.getEventStateLastTime(), stateMachineProcessor.getStateMachineData().getEventStateLastTime());
    // update expected time stamps to match actual for the times that are expected to change.
    expectedStateMachineData.setDownloadFailedSinceTime(stateMachineProcessor.getStateMachineData().getDownloadFailedSinceTime());
    expectedStateMachineData.setEventStateLastTime(stateMachineProcessor.getStateMachineData().getEventStateLastTime());

    EXPECT_PRED_FORMAT2(stateMachineDataIsEquivalent, expectedStateMachineData, stateMachineProcessor.getStateMachineData());

}

TEST_F(StateMachineProcessorTest, StateMachinesCorrectlyUpdatedWhenUpdateIsMultiplePackageMissing) // NOLINT
{
    std::string rawJsonStateMachineData = createJsonString(R"("InstallStateCredit": "3")", R"("InstallStateCredit": "1")");
    rawJsonStateMachineData = updateJsonString(rawJsonStateMachineData, R"("DownloadStateCredit": "72")", R"("DownloadStateCredit": "18")");

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData));

    stateMachinesModule::StateMachineProcessor stateMachineProcessor("1610465945");

    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::MULTIPLEPACKAGEMISSING);

    StateMachineData expectedStateMachineData = StateMachineData::fromJsonStateMachineData(createJsonString("", ""));

    expectedStateMachineData.setDownloadStateCredit("0");
    expectedStateMachineData.setInstallStateCredit("1");
    expectedStateMachineData.setDownloadState("1");
    expectedStateMachineData.setInstallState("0");
    expectedStateMachineData.setEventStateLastError(std::to_string(configModule::EventMessageNumber::MULTIPLEPACKAGEMISSING));

    EXPECT_NE(expectedStateMachineData.getDownloadFailedSinceTime(), stateMachineProcessor.getStateMachineData().getDownloadFailedSinceTime());
    EXPECT_NE(expectedStateMachineData.getEventStateLastTime(), stateMachineProcessor.getStateMachineData().getEventStateLastTime());
    // update expected time stamps to match actual for the times that are expected to change.
    expectedStateMachineData.setDownloadFailedSinceTime(stateMachineProcessor.getStateMachineData().getDownloadFailedSinceTime());
    expectedStateMachineData.setEventStateLastTime(stateMachineProcessor.getStateMachineData().getEventStateLastTime());

    EXPECT_PRED_FORMAT2(stateMachineDataIsEquivalent, expectedStateMachineData, stateMachineProcessor.getStateMachineData());

}

TEST_F(StateMachineProcessorTest, StateMachinesCorrectlyUpdatedWhenUpdateResultIsDownloadFailed) // NOLINT
{
    std::string rawJsonStateMachineData = createJsonString(R"("InstallStateCredit": "3")", R"("InstallStateCredit": "1")");
    rawJsonStateMachineData = updateJsonString(rawJsonStateMachineData, R"("DownloadStateCredit": "72")", R"("DownloadStateCredit": "18")");

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData));

    stateMachinesModule::StateMachineProcessor stateMachineProcessor("1610465945");

    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::DOWNLOADFAILED);

    StateMachineData expectedStateMachineData = StateMachineData::fromJsonStateMachineData(createJsonString("", ""));

    expectedStateMachineData.setDownloadStateCredit("0");
    expectedStateMachineData.setInstallStateCredit("1");
    expectedStateMachineData.setDownloadState("1");
    expectedStateMachineData.setInstallState("0");

    EXPECT_NE(expectedStateMachineData.getDownloadFailedSinceTime(), stateMachineProcessor.getStateMachineData().getDownloadFailedSinceTime());
    EXPECT_NE(expectedStateMachineData.getEventStateLastTime(), stateMachineProcessor.getStateMachineData().getEventStateLastTime());
    // update expected time stamps to match actual for the times that are expected to change.
    expectedStateMachineData.setDownloadFailedSinceTime(stateMachineProcessor.getStateMachineData().getDownloadFailedSinceTime());
    expectedStateMachineData.setEventStateLastTime(stateMachineProcessor.getStateMachineData().getEventStateLastTime());
    expectedStateMachineData.setEventStateLastError(std::to_string(configModule::EventMessageNumber::DOWNLOADFAILED));

    EXPECT_PRED_FORMAT2(stateMachineDataIsEquivalent, expectedStateMachineData, stateMachineProcessor.getStateMachineData());

}

TEST_F(StateMachineProcessorTest, StateMachinesCorrectlyUpdatedWhenFirstUpdateResultIsDownloadFailed) // NOLINT
{
    std::string rawJsonStateMachineData = createJsonString("", "");

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData));

    stateMachinesModule::StateMachineProcessor stateMachineProcessor("1610465945");

    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::DOWNLOADFAILED);

    StateMachineData expectedStateMachineData = StateMachineData::fromJsonStateMachineData(createJsonString("", ""));

    expectedStateMachineData.setDownloadStateCredit("54");
    expectedStateMachineData.setInstallStateCredit("3");
    expectedStateMachineData.setDownloadState("0");
    expectedStateMachineData.setInstallState("0");

    EXPECT_PRED_FORMAT2(stateMachineDataIsEquivalent, expectedStateMachineData, stateMachineProcessor.getStateMachineData());

}

TEST_F(StateMachineProcessorTest, StateMachinesCorrectlyUpdatedWhenUpdateResultIsDownloadFailedThenSuccess) // NOLINT
{
    std::string rawJsonStateMachineData = createJsonString(R"("InstallStateCredit": "3")", R"("InstallStateCredit": "1")");
    rawJsonStateMachineData = updateJsonString(rawJsonStateMachineData, R"("DownloadStateCredit": "72")", R"("DownloadStateCredit": "18")");

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData)).RetiresOnSaturation();

    stateMachinesModule::StateMachineProcessor stateMachineProcessor("1610465945");

    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::DOWNLOADFAILED);

    StateMachineData expectedStateMachineData = StateMachineData::fromJsonStateMachineData(createJsonString("", ""));

    expectedStateMachineData.setDownloadStateCredit("0");
    expectedStateMachineData.setInstallStateCredit("1");
    expectedStateMachineData.setDownloadState("1");
    expectedStateMachineData.setInstallState("0");

    EXPECT_NE(
        expectedStateMachineData.getDownloadFailedSinceTime(),
        stateMachineProcessor.getStateMachineData().getDownloadFailedSinceTime());
    EXPECT_NE(
        expectedStateMachineData.getEventStateLastTime(),
        stateMachineProcessor.getStateMachineData().getEventStateLastTime());
    // update expected time stamps to match actual for the times that are expected to change.
    expectedStateMachineData.setDownloadFailedSinceTime(
        stateMachineProcessor.getStateMachineData().getDownloadFailedSinceTime());
    expectedStateMachineData.setEventStateLastTime(
        stateMachineProcessor.getStateMachineData().getEventStateLastTime());
    expectedStateMachineData.setEventStateLastError(std::to_string(configModule::EventMessageNumber::DOWNLOADFAILED));

    EXPECT_PRED_FORMAT2(
        stateMachineDataIsEquivalent, expectedStateMachineData, stateMachineProcessor.getStateMachineData());

    // Test when next update is successful

    EXPECT_CALL(fileSystemMock, readFile(_))
        .WillOnce(Return(expectedStateMachineData.toJsonStateMachineData(expectedStateMachineData)));
    stateMachinesModule::StateMachineProcessor stateMachineProcessor2("1610465945"); // File will be loaded
    stateMachineProcessor2.updateStateMachines(configModule::EventMessageNumber::SUCCESS);

    expectedStateMachineData.setDownloadFailedSinceTime("0");
    expectedStateMachineData.setDownloadStateCredit("72");
    expectedStateMachineData.setDownloadState("0");

    // update expected time stamps to match actual for the times that are expected to change.
    expectedStateMachineData.setInstallFailedSinceTime(
        stateMachineProcessor2.getStateMachineData().getInstallFailedSinceTime());
    expectedStateMachineData.setInstallStateCredit("3");
    expectedStateMachineData.setEventStateLastError(std::to_string(configModule::EventMessageNumber::SUCCESS));

    EXPECT_PRED_FORMAT2(
        stateMachineDataIsEquivalent, expectedStateMachineData, stateMachineProcessor2.getStateMachineData());
}

TEST_F(StateMachineProcessorTest, StateMachinesCorrectlyUpdatedWhenUpdateResultIsConnectionError) // NOLINT
{
    std::string rawJsonStateMachineData = createJsonString(R"("InstallStateCredit": "3")", R"("InstallStateCredit": "1")");
    rawJsonStateMachineData = updateJsonString(rawJsonStateMachineData, R"("DownloadStateCredit": "72")", R"("DownloadStateCredit": "1")");

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData));

    stateMachinesModule::StateMachineProcessor stateMachineProcessor("1610465945");

    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::CONNECTIONERROR);

    StateMachineData expectedStateMachineData = StateMachineData::fromJsonStateMachineData(createJsonString("", ""));

    expectedStateMachineData.setDownloadStateCredit("0");
    expectedStateMachineData.setInstallStateCredit("1");
    expectedStateMachineData.setDownloadState("1");
    expectedStateMachineData.setInstallState("0");

    EXPECT_NE(expectedStateMachineData.getDownloadFailedSinceTime(), stateMachineProcessor.getStateMachineData().getDownloadFailedSinceTime());
    EXPECT_NE(expectedStateMachineData.getEventStateLastTime(), stateMachineProcessor.getStateMachineData().getEventStateLastTime());
    // update expected time stamps to match actual for the times that are expected to change.
    expectedStateMachineData.setDownloadFailedSinceTime(stateMachineProcessor.getStateMachineData().getDownloadFailedSinceTime());
    expectedStateMachineData.setEventStateLastTime(stateMachineProcessor.getStateMachineData().getEventStateLastTime());
    expectedStateMachineData.setEventStateLastError(std::to_string(configModule::EventMessageNumber::CONNECTIONERROR));

    EXPECT_PRED_FORMAT2(stateMachineDataIsEquivalent, expectedStateMachineData, stateMachineProcessor.getStateMachineData());

}

TEST_F(StateMachineProcessorTest, StateMachinesCorrectlyUpdatedWhenUpdateResultIsConnectionErrorThenSuccess) // NOLINT
{
    std::string rawJsonStateMachineData = createJsonString(R"("InstallStateCredit": "3")", R"("InstallStateCredit": "1")");
    rawJsonStateMachineData = updateJsonString(rawJsonStateMachineData, R"("DownloadStateCredit": "72")", R"("DownloadStateCredit": "1")");

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData)).RetiresOnSaturation();

    stateMachinesModule::StateMachineProcessor stateMachineProcessor("1610465945");

    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::CONNECTIONERROR);

    StateMachineData expectedStateMachineData = StateMachineData::fromJsonStateMachineData(createJsonString("", ""));

    expectedStateMachineData.setDownloadStateCredit("0");
    expectedStateMachineData.setInstallStateCredit("1");
    expectedStateMachineData.setDownloadState("1");
    expectedStateMachineData.setInstallState("0");

    EXPECT_NE(expectedStateMachineData.getDownloadFailedSinceTime(), stateMachineProcessor.getStateMachineData().getDownloadFailedSinceTime());
    EXPECT_NE(expectedStateMachineData.getEventStateLastTime(), stateMachineProcessor.getStateMachineData().getEventStateLastTime());
    // update expected time stamps to match actual for the times that are expected to change.
    expectedStateMachineData.setDownloadFailedSinceTime(stateMachineProcessor.getStateMachineData().getDownloadFailedSinceTime());
    expectedStateMachineData.setEventStateLastTime(stateMachineProcessor.getStateMachineData().getEventStateLastTime());
    expectedStateMachineData.setEventStateLastError(std::to_string(configModule::EventMessageNumber::CONNECTIONERROR));

    EXPECT_PRED_FORMAT2(stateMachineDataIsEquivalent, expectedStateMachineData, stateMachineProcessor.getStateMachineData());

    // Test when next update is successful

    EXPECT_CALL(fileSystemMock, readFile(_))
        .WillOnce(Return(expectedStateMachineData.toJsonStateMachineData(expectedStateMachineData)));
    stateMachinesModule::StateMachineProcessor stateMachineProcessor2("1610465945"); // File will be loaded
    stateMachineProcessor2.updateStateMachines(configModule::EventMessageNumber::SUCCESS);

    expectedStateMachineData.setDownloadFailedSinceTime("0");
    expectedStateMachineData.setDownloadStateCredit("72");
    expectedStateMachineData.setDownloadState("0");

    // update expected time stamps to match actual for the times that are expected to change.
    expectedStateMachineData.setInstallStateCredit("3");
    expectedStateMachineData.setEventStateLastError(std::to_string(configModule::EventMessageNumber::SUCCESS));

    EXPECT_PRED_FORMAT2(
        stateMachineDataIsEquivalent, expectedStateMachineData, stateMachineProcessor2.getStateMachineData());

}

TEST_F(StateMachineProcessorTest, StateMachinesCorrectlyUpdatedWhenUpdateResultIsInstallFailedAndThenFailsAgain) // NOLINT
{
    std::string rawJsonStateMachineData = createJsonString( R"("InstallStateCredit": "3")", R"("InstallStateCredit": "1")");

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData)).RetiresOnSaturation();

    stateMachinesModule::StateMachineProcessor stateMachineProcessor("1610465945");

    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::INSTALLFAILED);

    StateMachineData expectedStateMachineData = StateMachineData::fromJsonStateMachineData(createJsonString("", ""));
    expectedStateMachineData.setInstallStateCredit("0");
    expectedStateMachineData.setInstallState("1");
    expectedStateMachineData.setEventStateLastError(std::to_string(configModule::EventMessageNumber::INSTALLFAILED));

    EXPECT_NE(expectedStateMachineData.getInstallFailedSinceTime(), stateMachineProcessor.getStateMachineData().getInstallFailedSinceTime());
    EXPECT_NE(expectedStateMachineData.getEventStateLastTime(), stateMachineProcessor.getStateMachineData().getEventStateLastTime());
    // update expected time stamps to match actual for the times that are expected to change.
    expectedStateMachineData.setInstallFailedSinceTime(stateMachineProcessor.getStateMachineData().getInstallFailedSinceTime());
    expectedStateMachineData.setEventStateLastTime(stateMachineProcessor.getStateMachineData().getEventStateLastTime());

    EXPECT_PRED_FORMAT2(stateMachineDataIsEquivalent, expectedStateMachineData, stateMachineProcessor.getStateMachineData());

    // Fail update again

    EXPECT_CALL(fileSystemMock, readFile(_))
            .WillOnce(Return(expectedStateMachineData.toJsonStateMachineData(expectedStateMachineData)));
    stateMachinesModule::StateMachineProcessor stateMachineProcessor2("1610465945"); // File will be loaded
    stateMachineProcessor2.updateStateMachines(configModule::EventMessageNumber::INSTALLFAILED);

    // last failed time should not have changed.
    EXPECT_PRED_FORMAT2(
            stateMachineDataIsEquivalent, expectedStateMachineData, stateMachineProcessor2.getStateMachineData());
}

TEST_P(StateMachineProcessorTestWithCredit, StateMachinesCorrectlyUpdatedWhenCreditIsLargeOrNegative) // NOLINT
{
    std::string credit = std::get<0>(GetParam());
    long defaultValue = std::get<1>(GetParam());
    long testValue = std::get<2>(GetParam());
    int status = std::get<3>(GetParam());

    std::ostringstream oldPart, newPart;
    oldPart << "\"" << credit << "\": \"" << defaultValue << "\"";
    newPart << "\"" << credit << "\": \"" << testValue << "\"";
    std::string rawJsonStateMachineData = createJsonString(oldPart.str(), newPart.str());

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData)).RetiresOnSaturation();

    stateMachinesModule::StateMachineProcessor stateMachineProcessor("1610465945");

    stateMachineProcessor.updateStateMachines(status);

    switch (status)
    {
        case configModule::EventMessageNumber::INSTALLFAILED:
            {
                long expected = (testValue < 0) ? 0 : testValue - 1;
                EXPECT_EQ(stateMachineProcessor.getStateMachineData().getInstallStateCredit(), std::to_string(expected));
            }
            break;

        case configModule::EventMessageNumber::DOWNLOADFAILED:
            {
                long expected = (testValue < 0) ? 0 : testValue - 18;
                EXPECT_EQ(stateMachineProcessor.getStateMachineData().getDownloadStateCredit(), std::to_string(expected));
            }
            break;

        default:
            FAIL() << "unexpected status " << status;
            break;
    }
}

INSTANTIATE_TEST_CASE_P(StateMachineProcessorCreditTest, StateMachineProcessorTestWithCredit, ::testing::Values(
            std::make_tuple("InstallStateCredit", 3, -1, configModule::EventMessageNumber::INSTALLFAILED),
            std::make_tuple("InstallStateCredit", 3, 0x7fffffff, configModule::EventMessageNumber::INSTALLFAILED),
            std::make_tuple("DownloadStateCredit", 72, -1, configModule::EventMessageNumber::DOWNLOADFAILED),
            std::make_tuple("DownloadStateCredit", 72, 0x7fffffff, configModule::EventMessageNumber::DOWNLOADFAILED)));

TEST_P(StateMachineProcessorTestWithException, StateMachinesReadNotANumberShouldThrow) // NOLINT
{
    std::ostringstream rawJsonStateMachineData;
    rawJsonStateMachineData << "{\"" << std::get<0>(GetParam()) << "\":\"not a number\"}";
    std::string expectedException = std::get<1>(GetParam());

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData.str()));

    try
    {
        stateMachinesModule::StateMachineProcessor stateMachineProcessor("1610465945");
        FAIL();
    }
    catch (const std::exception& ex)
    {
        EXPECT_STREQ(expectedException.c_str(), ex.what());
    }
}

INSTANTIATE_TEST_CASE_P(StateMachineProcessorExceptionTest, StateMachineProcessorTestWithException, ::testing::Values(
            std::make_tuple("DownloadStateCredit", "stoi"),
            std::make_tuple("InstallStateCredit", "stoi"),
            std::make_tuple("EventStateLastError", "stoi"),
            std::make_tuple("DownloadFailedSinceTime", "stol"),
            std::make_tuple("EventStateLastTime", "stol"),
            std::make_tuple("InstallFailedSinceTime", "stol"),
            std::make_tuple("LastGoodInstallTime", "stol")));

TEST_F(StateMachineProcessorTest, StateMachinesReadCatchesFileSystemException) // NOLINT
{
    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Throw(IFileSystemException("permission denied")));

    EXPECT_NO_THROW(stateMachinesModule::StateMachineProcessor stateMachineProcessor("1610465945"));
}

TEST_F(StateMachineProcessorTest, StateMachinesReadCatchesFileTooLargeException) // NOLINT
{
    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Throw(IFileTooLargeException("file too large")));

    EXPECT_NO_THROW(stateMachinesModule::StateMachineProcessor stateMachineProcessor("1610465945"));
}

TEST_F(StateMachineProcessorTest, StateMachinesWriteCatchesFileSystemException) // NOLINT
{
    std::string rawJsonStateMachineData = createJsonString("", "");

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData));
    EXPECT_CALL(fileSystemMock, writeFile(_, _)).WillOnce(Throw(IFileSystemException("permission denied")));

    stateMachinesModule::StateMachineProcessor stateMachineProcessor("1610465945");

    EXPECT_NO_THROW(stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::SUCCESS));
}

TEST_F(StateMachineProcessorTest, EventStateMachineCorrectlyCanSendEventWhenResultIsDownloadFailed) // NOLINT
{
    std::string rawJsonStateMachineData = createJsonString("", "");

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData));

    stateMachinesModule::StateMachineProcessor stateMachineProcessor("1610465945");

    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::DOWNLOADFAILED);
    bool expectedCanSendEvent = false;
    EXPECT_EQ( expectedCanSendEvent, stateMachineProcessor.getStateMachineData().canSendEvent());
}


TEST_F(StateMachineProcessorTest, EventStateMachineCorrectlyCanSendEventWhenSuccess) // NOLINT
{
    std::string rawJsonStateMachineData = createJsonString("", "");

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData));

    stateMachinesModule::StateMachineProcessor stateMachineProcessor("1610465945");

    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::SUCCESS);
    bool expectedCanSendEvent = true;
    EXPECT_EQ( expectedCanSendEvent, stateMachineProcessor.getStateMachineData().canSendEvent());
}

TEST_F(StateMachineProcessorTest, EventStateMachineCorrectlyCanSendEventWhenPermanentFailure) // NOLINT
{
    std::string rawJsonStateMachineData = createJsonString("", "");

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData));

    stateMachinesModule::StateMachineProcessor stateMachineProcessor("1610465945");

    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::INSTALLFAILED);
    bool expectedCanSendEvent = false;
    EXPECT_EQ( expectedCanSendEvent, stateMachineProcessor.getStateMachineData().canSendEvent());
    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::INSTALLFAILED);
    bool expectedCanSendEvent2 = false;
    EXPECT_EQ( expectedCanSendEvent2, stateMachineProcessor.getStateMachineData().canSendEvent());
    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::INSTALLFAILED);
    bool expectedCanSendEvent3 = true;
    EXPECT_EQ( expectedCanSendEvent3, stateMachineProcessor.getStateMachineData().canSendEvent());
}

TEST_F(StateMachineProcessorTest, EventStateMachineCorrectlyCanSendEventWhenPermanentFailureAndSuccess) // NOLINT
{
    std::string rawJsonStateMachineData = createJsonString("", "");

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData));

    stateMachinesModule::StateMachineProcessor stateMachineProcessor("1610465945");

    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::INSTALLFAILED);
    bool expectedCanSendEvent = false;
    EXPECT_EQ( expectedCanSendEvent, stateMachineProcessor.getStateMachineData().canSendEvent());
    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::INSTALLFAILED);
    bool expectedCanSendEvent2 = false;
    EXPECT_EQ( expectedCanSendEvent2, stateMachineProcessor.getStateMachineData().canSendEvent());
    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::INSTALLFAILED);
    bool expectedCanSendEvent3 = true;
    EXPECT_EQ( expectedCanSendEvent3, stateMachineProcessor.getStateMachineData().canSendEvent());
    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::SUCCESS);
    bool expectedCanSendEvent4 = true;
    EXPECT_EQ( expectedCanSendEvent4, stateMachineProcessor.getStateMachineData().canSendEvent());
}

TEST_F(StateMachineProcessorTest, EventStateMachineCorrectlyThrottlesPermanentSuccess) // NOLINT
{
    std::string rawJsonStateMachineData = createJsonString("", "");

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData));

    stateMachinesModule::StateMachineProcessor stateMachineProcessor("1610465945");

    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::SUCCESS);
    bool expectedCanSendEvent = true;
    EXPECT_EQ( expectedCanSendEvent, stateMachineProcessor.getStateMachineData().canSendEvent());
    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::SUCCESS);
    bool expectedCanSendEvent2 = false;
    EXPECT_EQ( expectedCanSendEvent2, stateMachineProcessor.getStateMachineData().canSendEvent());
    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::SUCCESS);
    bool expectedCanSendEvent3 = false;
    EXPECT_EQ( expectedCanSendEvent3, stateMachineProcessor.getStateMachineData().canSendEvent());
}

TEST_F(StateMachineProcessorTest, EventStateMachineCanSendEventIsFalseWhenPermanentSuccessWithin24Hours) // NOLINT
{
    auto currentTime = std::chrono::system_clock::now();
    int secondsInDay = 86400;
    int timeDiff = secondsInDay - 5;
    std::string epoch = std::to_string(std::chrono::duration_cast<std::chrono::seconds>(currentTime.time_since_epoch()).count() - timeDiff);
    std::stringstream newEpochString;
    newEpochString << "\"EventStateLastTime\": \"" << epoch << "\"";
    std::string rawJsonStateMachineData = createJsonString(R"("EventStateLastTime": "1610440731")", newEpochString.str());

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData));

    stateMachinesModule::StateMachineProcessor stateMachineProcessor(epoch);

    auto stateMachineData = stateMachineProcessor.getStateMachineData();

    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::SUCCESS);

    auto stateMachineData2 = stateMachineProcessor.getStateMachineData();
    // Testing will load previous success state, therefore the next success states should report false for canSendEvent.
    bool expectedCanSendEvent = false;
    EXPECT_EQ( expectedCanSendEvent, stateMachineProcessor.getStateMachineData().canSendEvent());
}

TEST_F(StateMachineProcessorTest, EventStateMachineCanSendEventIsTrueWhenPermanentSuccessAfter24Hours) // NOLINT
{
    auto currentTime = std::chrono::system_clock::now();
    int secondsInDay = 86400;
    int timeDiff = secondsInDay + 5;
    std::string epoch = std::to_string(std::chrono::duration_cast<std::chrono::seconds>(currentTime.time_since_epoch()).count() - timeDiff);
    std::stringstream newEpochString;
    newEpochString << "\"EventStateLastTime\": \"" << epoch << "\"";
    std::string rawJsonStateMachineData = createJsonString(R"("EventStateLastTime": "1610440731")", newEpochString.str());

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData));

    stateMachinesModule::StateMachineProcessor stateMachineProcessor(epoch);

    auto stateMachineData = stateMachineProcessor.getStateMachineData();

    // Testing will load previous success state and the next success state should report canSendEvent = true when the last
    // event sent is older than 24 hours.
    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::SUCCESS);

    auto stateMachineData2 = stateMachineProcessor.getStateMachineData();
    bool expectedCanSendEvent = true;
    EXPECT_EQ( expectedCanSendEvent, stateMachineProcessor.getStateMachineData().canSendEvent());
}

TEST_F(StateMachineProcessorTest, EventStateMachineCanSendEventIsFalseWhenPermanentFailureWithin24Hours) // NOLINT
{
    auto currentTime = std::chrono::system_clock::now();
    int secondsInDay = 86400;
    int timeDiff = secondsInDay - 5;
    std::string epoch = std::to_string(std::chrono::duration_cast<std::chrono::seconds>(currentTime.time_since_epoch()).count() - timeDiff);
    std::stringstream newEpochString;
    newEpochString << "\"EventStateLastTime\": \"" << epoch << "\"";
    std::string rawJsonStateMachineData = createJsonString(R"("EventStateLastTime": "1610440731")", newEpochString.str());
    rawJsonStateMachineData = updateJsonString(rawJsonStateMachineData,  R"("InstallStateCredit": "3")", R"("InstallStateCredit": "0")");

    std::stringstream newEventErrorString;
    newEventErrorString << "\"EventStateLastError\": \"" << configModule::EventMessageNumber::INSTALLFAILED << "\"";
    rawJsonStateMachineData =
        updateJsonString(rawJsonStateMachineData,  R"("EventStateLastError": "0")", newEventErrorString.str());

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData));

    stateMachinesModule::StateMachineProcessor stateMachineProcessor(epoch);

    auto stateMachineData = stateMachineProcessor.getStateMachineData();

    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::INSTALLFAILED);

    auto stateMachineData2 = stateMachineProcessor.getStateMachineData();
    // Testing will load previous success state, therefore the next success states should report false for canSendEvent.
    bool expectedCanSendEvent = false;
    EXPECT_EQ( expectedCanSendEvent, stateMachineProcessor.getStateMachineData().canSendEvent());

}

TEST_F(StateMachineProcessorTest, EventStateMachineCanSendEventIsTrueWhenPermanentFailureAfter24Hours) // NOLINT
{
    auto currentTime = std::chrono::system_clock::now();
    int secondsInDay = 86400;
    int timeDiff = secondsInDay + 5;
    std::string epoch = std::to_string(std::chrono::duration_cast<std::chrono::seconds>(currentTime.time_since_epoch()).count() - timeDiff);
    std::stringstream newEpochString;
    newEpochString << "\"EventStateLastTime\": \"" << epoch << "\"";
    std::string rawJsonStateMachineData = createJsonString(R"("EventStateLastTime": "1610440731")", newEpochString.str());
    rawJsonStateMachineData = updateJsonString(rawJsonStateMachineData,  R"("InstallStateCredit": "3")", R"("InstallStateCredit": "0")");

    std::stringstream newEventErrorString;
    newEventErrorString << "\"EventStateLastError\": \"" << configModule::EventMessageNumber::INSTALLFAILED << "\"";
    rawJsonStateMachineData =
        updateJsonString(rawJsonStateMachineData,  R"("EventStateLastError": "0")", newEventErrorString.str());

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData));

    stateMachinesModule::StateMachineProcessor stateMachineProcessor(epoch);

    auto stateMachineData = stateMachineProcessor.getStateMachineData();

    // Testing will load previous success state and the next success state should report canSendEvent = true when the last
    // event sent is older than 24 hours.
    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::INSTALLFAILED);

    auto stateMachineData2 = stateMachineProcessor.getStateMachineData();
    bool expectedCanSendEvent = true;
    EXPECT_EQ( expectedCanSendEvent, stateMachineProcessor.getStateMachineData().canSendEvent());
}

TEST_F(StateMachineProcessorTest, EventStateMachineCanSendEventIsFalseWhenPermanentConnectionErrorLess72Hours) // NOLINT
{
    // Connection error reduce Download credit by 1.
    // Download credit starts at 72.  Assumption updates 1 every hour.
    // Test simulates Connection error will not be sent when download credit is grater then 0.
    auto currentTime = std::chrono::system_clock::now();
    int secondsInDay = 60 * 60 *72;
    int timeDiff = secondsInDay - 5;
    std::string epoch = std::to_string(std::chrono::duration_cast<std::chrono::seconds>(currentTime.time_since_epoch()).count() - timeDiff);
    std::stringstream newEpochString;
    newEpochString << "\"EventStateLastTime\": \"" << epoch << "\"";
    std::string rawJsonStateMachineData = createJsonString(R"("EventStateLastTime": "1610440731")", newEpochString.str());
    rawJsonStateMachineData = updateJsonString(rawJsonStateMachineData,  R"("DownloadStateCredit": "72")", R"("DownloadStateCredit": "2")");

    std::stringstream newEventErrorString;
    newEventErrorString << "\"EventStateLastError\": \"" << configModule::EventMessageNumber::CONNECTIONERROR << "\"";
    rawJsonStateMachineData =
        updateJsonString(rawJsonStateMachineData,  R"("EventStateLastError": "0")", newEventErrorString.str());

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData));

    stateMachinesModule::StateMachineProcessor stateMachineProcessor(epoch);

    auto stateMachineData = stateMachineProcessor.getStateMachineData();

    // Testing will load previous success state and the next success state should report canSendEvent = true when the last
    // event sent is older than 24 hours.
    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::CONNECTIONERROR);
    auto stateMachineData2 = stateMachineProcessor.getStateMachineData();
    bool expectedCanSendEvent = false;
    EXPECT_EQ( expectedCanSendEvent, stateMachineProcessor.getStateMachineData().canSendEvent());
}

TEST_F(StateMachineProcessorTest, EventStateMachineCanSendEventIsTrueWhenPermanentConnectionErrorOver72Hours) // NOLINT
{
    // Connection error reduce Download credit by 1.
    // Download credit starts at 72.  Assumption updates 1 every hour.
    // Test simulates Connection error will not be sent when download credit is grater then 0.
    auto currentTime = std::chrono::system_clock::now();
    int secondsInDay = 60 * 60 *72;
    int timeDiff = secondsInDay + 5;
    std::string epoch = std::to_string(std::chrono::duration_cast<std::chrono::seconds>(currentTime.time_since_epoch()).count() - timeDiff);
    std::stringstream newEpochString;
    newEpochString << "\"EventStateLastTime\": \"" << epoch << "\"";
    std::string rawJsonStateMachineData = createJsonString(R"("EventStateLastTime": "1610440731")", newEpochString.str());
    rawJsonStateMachineData = updateJsonString(rawJsonStateMachineData,  R"("DownloadStateCredit": "72")", R"("DownloadStateCredit": "0")");

    std::stringstream newEventErrorString;
    newEventErrorString << "\"EventStateLastError\": \"" << configModule::EventMessageNumber::CONNECTIONERROR << "\"";
    rawJsonStateMachineData =
        updateJsonString(rawJsonStateMachineData,  R"("EventStateLastError": "0")", newEventErrorString.str());

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData));

    stateMachinesModule::StateMachineProcessor stateMachineProcessor(epoch);

    auto stateMachineData = stateMachineProcessor.getStateMachineData();

    // Testing will load previous success state and the next success state should report canSendEvent = true when the last
    // event sent is older than 24 hours.
    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::CONNECTIONERROR);
    auto stateMachineData2 = stateMachineProcessor.getStateMachineData();
    bool expectedCanSendEvent = true;
    EXPECT_EQ( expectedCanSendEvent, stateMachineProcessor.getStateMachineData().canSendEvent());
}

TEST_F(StateMachineProcessorTest, EventStateMachinesCorrectlyUpdatedWhenFirstInstallUpdateResultIsSuccess) // NOLINT
{
    auto& fileSystemMock = setupFileSystemAndGetMock(false);
    EXPECT_CALL(fileSystemMock, readFile(_)).Times(0);

    stateMachinesModule::StateMachineProcessor stateMachineProcessor("1610465945");

    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::SUCCESS);

    StateMachineData expectedStateMachineData = StateMachineData::fromJsonStateMachineData(createJsonString("", ""));
    EXPECT_NE( expectedStateMachineData.getEventStateLastTime(),stateMachineProcessor.getStateMachineData().getEventStateLastTime());
    expectedStateMachineData.setEventStateLastTime(stateMachineProcessor.getStateMachineData().getEventStateLastTime());
    expectedStateMachineData.setCanSendEvent(true);

    EXPECT_PRED_FORMAT2(stateMachineDataIsEquivalent, expectedStateMachineData, stateMachineProcessor.getStateMachineData());

}

TEST_F(StateMachineProcessorTest, EventStateMachinesCorrectlyUpdatedWhenFirstInstallUpdateResultInFailure) // NOLINT
{
    auto& fileSystemMock = setupFileSystemAndGetMock(false);
    EXPECT_CALL(fileSystemMock, readFile(_)).Times(0);

    stateMachinesModule::StateMachineProcessor stateMachineProcessor("1610465945");

    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::DOWNLOADFAILED);

    StateMachineData expectedStateMachineData = StateMachineData::fromJsonStateMachineData(createDefaultJsonString("", ""));
    EXPECT_NE( expectedStateMachineData.getEventStateLastTime(),stateMachineProcessor.getStateMachineData().getEventStateLastTime());
    // Updated expected values.
    expectedStateMachineData.setEventStateLastTime(stateMachineProcessor.getStateMachineData().getEventStateLastTime());
    expectedStateMachineData.setEventStateLastError(std::to_string(configModule::EventMessageNumber::DOWNLOADFAILED));
    expectedStateMachineData.setLastGoodInstallTime("1610465945");
    expectedStateMachineData.setCanSendEvent(true);

    EXPECT_PRED_FORMAT2(stateMachineDataIsEquivalent, expectedStateMachineData, stateMachineProcessor.getStateMachineData());
}

TEST_F(StateMachineProcessorTest, EventStateMachinesCorrectlyUpdatedWhenFirstInstallUpdateResultInConnectionFailure) // NOLINT
{
    auto& fileSystemMock = setupFileSystemAndGetMock(false);
    EXPECT_CALL(fileSystemMock, readFile(_)).Times(0);

    stateMachinesModule::StateMachineProcessor stateMachineProcessor("1610465945");

    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::CONNECTIONERROR);
    StateMachineData temp = stateMachineProcessor.getStateMachineData();

    StateMachineData expectedStateMachineData = StateMachineData::fromJsonStateMachineData(createDefaultJsonString("", ""));
    //EXPECT_NE( expectedStateMachineData.getEventStateLastTime(),stateMachineProcessor.getStateMachineData().getEventStateLastTime());
    // Updated expected values.
    expectedStateMachineData.setEventStateLastTime(stateMachineProcessor.getStateMachineData().getEventStateLastTime());
    expectedStateMachineData.setEventStateLastError(std::to_string(configModule::EventMessageNumber::CONNECTIONERROR));
    expectedStateMachineData.setLastGoodInstallTime("1610465945");
    expectedStateMachineData.setCanSendEvent(true);

    EXPECT_PRED_FORMAT2(stateMachineDataIsEquivalent, expectedStateMachineData, stateMachineProcessor.getStateMachineData());

}