/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "StateMachineDataBase.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
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

TEST_F(StateMachineProcessorTest, StateMachinesCorrectlyUpdatedWhenUpdateResultIsSuccess) // NOLINT
{
    std::string rawJsonStateMachineData = createJsonString("", "");

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData));

    stateMachinesModule::StateMachineProcessor stateMachineProcessor;

    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::SUCCESS);

    StateMachineData expectedStateMachineData = StateMachineData::fromJsonStateMachineData(createJsonString("", ""));

    EXPECT_NE(expectedStateMachineData.getLastGoodInstallTime(), stateMachineProcessor.getStateMachineData().getLastGoodInstallTime());
    // update expected time stamps to match actual for the times that are expected to change.
    expectedStateMachineData.setLastGoodInstallTime(stateMachineProcessor.getStateMachineData().getLastGoodInstallTime());

    EXPECT_PRED_FORMAT2(stateMachineDataIsEquivalent, expectedStateMachineData, stateMachineProcessor.getStateMachineData());

}

TEST_F(StateMachineProcessorTest, StateMachinesCorrectlyUpdatedWhenUpdateResultIsInstallFailed) // NOLINT
{
    std::string rawJsonStateMachineData = createJsonString("", "");

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData));

    stateMachinesModule::StateMachineProcessor stateMachineProcessor;

    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::INSTALLFAILED);

    std::string oldString = R"("InstallStateCredit": "3")";
    std::string newString = R"("InstallStateCredit": "2")";

    StateMachineData expectedStateMachineData = StateMachineData::fromJsonStateMachineData(createJsonString(oldString, newString));

    EXPECT_NE(expectedStateMachineData.getInstallFailedSinceTime(), stateMachineProcessor.getStateMachineData().getInstallFailedSinceTime());
    // update expected time stamps to match actual for the times that are expected to change.
    expectedStateMachineData.setInstallFailedSinceTime(stateMachineProcessor.getStateMachineData().getInstallFailedSinceTime());

    EXPECT_PRED_FORMAT2(stateMachineDataIsEquivalent, expectedStateMachineData, stateMachineProcessor.getStateMachineData());

}

TEST_F(StateMachineProcessorTest, StateMachinesCorrectlyUpdatedWhenUpdateResultIsInstallFailedThenSucceeded) // NOLINT
{
    // Test when next update fails
    std::string rawJsonStateMachineData = createJsonString("", "");

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData)).RetiresOnSaturation();

    stateMachinesModule::StateMachineProcessor stateMachineProcessor1;

    stateMachineProcessor1.updateStateMachines(configModule::EventMessageNumber::INSTALLFAILED);

    std::string oldString = R"("InstallStateCredit": "3")";
    std::string newString = R"("InstallStateCredit": "2")";

    StateMachineData expectedStateMachineData = StateMachineData::fromJsonStateMachineData(createJsonString(oldString, newString));

    EXPECT_NE(expectedStateMachineData.getInstallFailedSinceTime(),
        stateMachineProcessor1.getStateMachineData().getInstallFailedSinceTime());
    // update expected time stamps to match actual for the times that are expected to change.
    expectedStateMachineData.setInstallFailedSinceTime(
        stateMachineProcessor1.getStateMachineData().getInstallFailedSinceTime());

    EXPECT_PRED_FORMAT2(stateMachineDataIsEquivalent, expectedStateMachineData, stateMachineProcessor1.getStateMachineData());

    // Test when next update is successful

    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(expectedStateMachineData.toJsonStateMachineData(expectedStateMachineData)));
    stateMachinesModule::StateMachineProcessor stateMachineProcessor2; //File will be loaded
    stateMachineProcessor2.updateStateMachines(configModule::EventMessageNumber::SUCCESS);

    EXPECT_NE(expectedStateMachineData.getInstallFailedSinceTime(),stateMachineProcessor2.getStateMachineData().getInstallFailedSinceTime());

    EXPECT_NE(expectedStateMachineData.getLastGoodInstallTime(), stateMachineProcessor2.getStateMachineData().getLastGoodInstallTime());

    expectedStateMachineData.setLastGoodInstallTime(stateMachineProcessor2.getStateMachineData().getLastGoodInstallTime());

    // update expected time stamps to match actual for the times that are expected to change.
    expectedStateMachineData.setInstallFailedSinceTime(stateMachineProcessor2.getStateMachineData().getInstallFailedSinceTime());
    expectedStateMachineData.setInstallStateCredit("3");

    EXPECT_PRED_FORMAT2(stateMachineDataIsEquivalent, expectedStateMachineData, stateMachineProcessor2.getStateMachineData());

}




TEST_F(StateMachineProcessorTest, StateMachinesCorrectlyUpdatedWhenUpdateResultIsUpdateSourceMissing) // NOLINT
{
    std::string rawJsonStateMachineData = createJsonString("", "");

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData));

    stateMachinesModule::StateMachineProcessor stateMachineProcessor;

    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::UPDATESOURCEMISSING);

    StateMachineData expectedStateMachineData = StateMachineData::fromJsonStateMachineData(createJsonString("", ""));

    expectedStateMachineData.setDownloadStateCredit("54");
    expectedStateMachineData.setInstallStateCredit("2");

    EXPECT_NE(expectedStateMachineData.getDownloadFailedSinceTime(), stateMachineProcessor.getStateMachineData().getDownloadFailedSinceTime());
    EXPECT_NE(expectedStateMachineData.getInstallFailedSinceTime(), stateMachineProcessor.getStateMachineData().getInstallFailedSinceTime());
    // update expected time stamps to match actual for the times that are expected to change.
    expectedStateMachineData.setDownloadFailedSinceTime(stateMachineProcessor.getStateMachineData().getDownloadFailedSinceTime());
    expectedStateMachineData.setInstallFailedSinceTime(stateMachineProcessor.getStateMachineData().getInstallFailedSinceTime());

    EXPECT_PRED_FORMAT2(stateMachineDataIsEquivalent, expectedStateMachineData, stateMachineProcessor.getStateMachineData());
}

TEST_F(StateMachineProcessorTest, StateMachinesCorrectlyUpdatedWhenUpdateResultIstSinglePackageMissing) // NOLINT
{
    std::string rawJsonStateMachineData = createJsonString("", "");

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData));

    stateMachinesModule::StateMachineProcessor stateMachineProcessor;

    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::SINGLEPACKAGEMISSING);

    StateMachineData expectedStateMachineData = StateMachineData::fromJsonStateMachineData(createJsonString("", ""));

    expectedStateMachineData.setDownloadStateCredit("54");
    expectedStateMachineData.setInstallStateCredit("2");

    EXPECT_NE(expectedStateMachineData.getDownloadFailedSinceTime(), stateMachineProcessor.getStateMachineData().getDownloadFailedSinceTime());
    EXPECT_NE(expectedStateMachineData.getInstallFailedSinceTime(), stateMachineProcessor.getStateMachineData().getInstallFailedSinceTime());
    // update expected time stamps to match actual for the times that are expected to change.
    expectedStateMachineData.setDownloadFailedSinceTime(stateMachineProcessor.getStateMachineData().getDownloadFailedSinceTime());
    expectedStateMachineData.setInstallFailedSinceTime(stateMachineProcessor.getStateMachineData().getInstallFailedSinceTime());

    EXPECT_PRED_FORMAT2(stateMachineDataIsEquivalent, expectedStateMachineData, stateMachineProcessor.getStateMachineData());

}

TEST_F(StateMachineProcessorTest, StateMachinesCorrectlyUpdatedWhenUpdateIsMultiplePackageMissing) // NOLINT
{
    std::string rawJsonStateMachineData = createJsonString("", "");

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData));

    stateMachinesModule::StateMachineProcessor stateMachineProcessor;

    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::MULTIPLEPACKAGEMISSING);

    StateMachineData expectedStateMachineData = StateMachineData::fromJsonStateMachineData(createJsonString("", ""));

    expectedStateMachineData.setDownloadStateCredit("54");
    expectedStateMachineData.setInstallStateCredit("2");

    EXPECT_NE(expectedStateMachineData.getDownloadFailedSinceTime(), stateMachineProcessor.getStateMachineData().getDownloadFailedSinceTime());
    EXPECT_NE(expectedStateMachineData.getInstallFailedSinceTime(), stateMachineProcessor.getStateMachineData().getInstallFailedSinceTime());
    // update expected time stamps to match actual for the times that are expected to change.
    expectedStateMachineData.setDownloadFailedSinceTime(stateMachineProcessor.getStateMachineData().getDownloadFailedSinceTime());
    expectedStateMachineData.setInstallFailedSinceTime(stateMachineProcessor.getStateMachineData().getInstallFailedSinceTime());

    EXPECT_PRED_FORMAT2(stateMachineDataIsEquivalent, expectedStateMachineData, stateMachineProcessor.getStateMachineData());

}

TEST_F(StateMachineProcessorTest, StateMachinesCorrectlyUpdatedWhenUpdateResultIsDownloadFailed) // NOLINT
{
    std::string rawJsonStateMachineData = createJsonString("", "");

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData));

    stateMachinesModule::StateMachineProcessor stateMachineProcessor;

    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::DOWNLOADFAILED);

    StateMachineData expectedStateMachineData = StateMachineData::fromJsonStateMachineData(createJsonString("", ""));

    expectedStateMachineData.setDownloadStateCredit("54");
    expectedStateMachineData.setInstallStateCredit("2");

    EXPECT_NE(expectedStateMachineData.getDownloadFailedSinceTime(), stateMachineProcessor.getStateMachineData().getDownloadFailedSinceTime());
    EXPECT_NE(expectedStateMachineData.getInstallFailedSinceTime(), stateMachineProcessor.getStateMachineData().getInstallFailedSinceTime());
    // update expected time stamps to match actual for the times that are expected to change.
    expectedStateMachineData.setDownloadFailedSinceTime(stateMachineProcessor.getStateMachineData().getDownloadFailedSinceTime());
    expectedStateMachineData.setInstallFailedSinceTime(stateMachineProcessor.getStateMachineData().getInstallFailedSinceTime());

    EXPECT_PRED_FORMAT2(stateMachineDataIsEquivalent, expectedStateMachineData, stateMachineProcessor.getStateMachineData());

}

TEST_F(StateMachineProcessorTest, StateMachinesCorrectlyUpdatedWhenUpdateResultIsDownloadFailedThenSuccess) // NOLINT
{
    std::string rawJsonStateMachineData = createJsonString("", "");

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData)).RetiresOnSaturation();

    stateMachinesModule::StateMachineProcessor stateMachineProcessor;

    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::DOWNLOADFAILED);

    StateMachineData expectedStateMachineData = StateMachineData::fromJsonStateMachineData(createJsonString("", ""));

    expectedStateMachineData.setDownloadStateCredit("54");
    expectedStateMachineData.setInstallStateCredit("2");

    EXPECT_NE(
        expectedStateMachineData.getDownloadFailedSinceTime(),
        stateMachineProcessor.getStateMachineData().getDownloadFailedSinceTime());
    EXPECT_NE(
        expectedStateMachineData.getInstallFailedSinceTime(),
        stateMachineProcessor.getStateMachineData().getInstallFailedSinceTime());
    // update expected time stamps to match actual for the times that are expected to change.
    expectedStateMachineData.setDownloadFailedSinceTime(
        stateMachineProcessor.getStateMachineData().getDownloadFailedSinceTime());
    expectedStateMachineData.setInstallFailedSinceTime(
        stateMachineProcessor.getStateMachineData().getInstallFailedSinceTime());

    EXPECT_PRED_FORMAT2(
        stateMachineDataIsEquivalent, expectedStateMachineData, stateMachineProcessor.getStateMachineData());

    // Test when next update is successful

    EXPECT_CALL(fileSystemMock, readFile(_))
        .WillOnce(Return(expectedStateMachineData.toJsonStateMachineData(expectedStateMachineData)));
    stateMachinesModule::StateMachineProcessor stateMachineProcessor2; // File will be loaded
    stateMachineProcessor2.updateStateMachines(configModule::EventMessageNumber::SUCCESS);

    EXPECT_NE(
        expectedStateMachineData.getInstallFailedSinceTime(),
        stateMachineProcessor2.getStateMachineData().getInstallFailedSinceTime());

    EXPECT_NE(
        expectedStateMachineData.getLastGoodInstallTime(),
        stateMachineProcessor2.getStateMachineData().getLastGoodInstallTime());

    expectedStateMachineData.setLastGoodInstallTime(
        stateMachineProcessor2.getStateMachineData().getLastGoodInstallTime());
    expectedStateMachineData.setDownloadFailedSinceTime("0");
    expectedStateMachineData.setDownloadStateCredit("72");

    // update expected time stamps to match actual for the times that are expected to change.
    expectedStateMachineData.setInstallFailedSinceTime(
        stateMachineProcessor2.getStateMachineData().getInstallFailedSinceTime());
    expectedStateMachineData.setInstallStateCredit("3");

    EXPECT_PRED_FORMAT2(
        stateMachineDataIsEquivalent, expectedStateMachineData, stateMachineProcessor2.getStateMachineData());
}

TEST_F(StateMachineProcessorTest, StateMachinesCorrectlyUpdatedWhenUpdateResultIsConnectionError) // NOLINT
{
    std::string rawJsonStateMachineData = createJsonString("", "");

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData));

    stateMachinesModule::StateMachineProcessor stateMachineProcessor;

    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::CONNECTIONERROR);

    StateMachineData expectedStateMachineData = StateMachineData::fromJsonStateMachineData(createJsonString("", ""));

    expectedStateMachineData.setDownloadStateCredit("71");
    expectedStateMachineData.setInstallStateCredit("2");

    EXPECT_NE(expectedStateMachineData.getDownloadFailedSinceTime(), stateMachineProcessor.getStateMachineData().getDownloadFailedSinceTime());
    EXPECT_NE(expectedStateMachineData.getInstallFailedSinceTime(), stateMachineProcessor.getStateMachineData().getInstallFailedSinceTime());
    // update expected time stamps to match actual for the times that are expected to change.
    expectedStateMachineData.setDownloadFailedSinceTime(stateMachineProcessor.getStateMachineData().getDownloadFailedSinceTime());
    expectedStateMachineData.setInstallFailedSinceTime(stateMachineProcessor.getStateMachineData().getInstallFailedSinceTime());

    EXPECT_PRED_FORMAT2(stateMachineDataIsEquivalent, expectedStateMachineData, stateMachineProcessor.getStateMachineData());

}

TEST_F(StateMachineProcessorTest, StateMachinesCorrectlyUpdatedWhenUpdateResultIsConnectionErrorThenSuccess) // NOLINT
{
    std::string rawJsonStateMachineData = createJsonString("", "");

    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, readFile(_)).WillOnce(Return(rawJsonStateMachineData)).RetiresOnSaturation();

    stateMachinesModule::StateMachineProcessor stateMachineProcessor;

    stateMachineProcessor.updateStateMachines(configModule::EventMessageNumber::CONNECTIONERROR);

    StateMachineData expectedStateMachineData = StateMachineData::fromJsonStateMachineData(createJsonString("", ""));

    expectedStateMachineData.setDownloadStateCredit("71");
    expectedStateMachineData.setInstallStateCredit("2");

    EXPECT_NE(expectedStateMachineData.getDownloadFailedSinceTime(), stateMachineProcessor.getStateMachineData().getDownloadFailedSinceTime());
    EXPECT_NE(expectedStateMachineData.getInstallFailedSinceTime(), stateMachineProcessor.getStateMachineData().getInstallFailedSinceTime());
    // update expected time stamps to match actual for the times that are expected to change.
    expectedStateMachineData.setDownloadFailedSinceTime(stateMachineProcessor.getStateMachineData().getDownloadFailedSinceTime());
    expectedStateMachineData.setInstallFailedSinceTime(stateMachineProcessor.getStateMachineData().getInstallFailedSinceTime());

    EXPECT_PRED_FORMAT2(stateMachineDataIsEquivalent, expectedStateMachineData, stateMachineProcessor.getStateMachineData());

    // Test when next update is successful

    EXPECT_CALL(fileSystemMock, readFile(_))
        .WillOnce(Return(expectedStateMachineData.toJsonStateMachineData(expectedStateMachineData)));
    stateMachinesModule::StateMachineProcessor stateMachineProcessor2; // File will be loaded
    stateMachineProcessor2.updateStateMachines(configModule::EventMessageNumber::SUCCESS);

    EXPECT_NE(
        expectedStateMachineData.getInstallFailedSinceTime(),
        stateMachineProcessor2.getStateMachineData().getInstallFailedSinceTime());

    EXPECT_NE(
        expectedStateMachineData.getLastGoodInstallTime(),
        stateMachineProcessor2.getStateMachineData().getLastGoodInstallTime());

    expectedStateMachineData.setLastGoodInstallTime(
        stateMachineProcessor2.getStateMachineData().getLastGoodInstallTime());
    expectedStateMachineData.setDownloadFailedSinceTime("0");
    expectedStateMachineData.setDownloadStateCredit("72");

    // update expected time stamps to match actual for the times that are expected to change.
    expectedStateMachineData.setInstallFailedSinceTime(
        stateMachineProcessor2.getStateMachineData().getInstallFailedSinceTime());
    expectedStateMachineData.setInstallStateCredit("3");

    EXPECT_PRED_FORMAT2(
        stateMachineDataIsEquivalent, expectedStateMachineData, stateMachineProcessor2.getStateMachineData());

}
