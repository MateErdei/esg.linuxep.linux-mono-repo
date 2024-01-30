// Copyright 2018-2023 Sophos Limited. All rights reserved.
#include "DownloadReportTestBuilder.h"

#include "Common/FileSystemImpl/FileSystemImpl.h"
#include "Common/Logging/ConsoleLoggingSetup.h"
#include "UpdateSchedulerImpl/configModule/DownloadReportsAnalyser.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MockFileSystem.h"

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

using namespace UpdateSchedulerImpl;
using namespace UpdateSchedulerImpl::configModule;
using namespace UpdateScheduler;
using Common::DownloadReport::DownloadReport;
using Common::DownloadReport::RepositoryStatus;

class TestDownloadReportAnalyser : public ::testing::Test
{
public:
    ::testing::AssertionResult insertMessagesAreEquivalent(
        const char* m_expr,
        const char* n_expr,
        const std::vector<MessageInsert>& expected,
        const std::vector<MessageInsert>& resulted)
    {
        std::stringstream s;
        s << m_expr << " and " << n_expr << " failed: ";

        if (expected.size() != resulted.size())
        {
            return ::testing::AssertionFailure() << s.str() << " messages insert differ. Expected " << expected.size()
                                                 << " messages, but received: " << resulted.size() << ".";
        }

        for (size_t i = 0; i < expected.size(); i++)
        {
            auto& expectedInsert = expected[i];
            auto& resultedInsert = resulted[i];
            if (expectedInsert.PackageName != resultedInsert.PackageName)
            {
                return ::testing::AssertionFailure()
                       << s.str() << " package name differ. Expected " << expectedInsert.PackageName
                       << " received: " << resultedInsert.PackageName << ".";
            }
            if (expectedInsert.ErrorDetails != resultedInsert.ErrorDetails)
            {
                return ::testing::AssertionFailure()
                       << s.str() << " error details differ. Expected " << expectedInsert.ErrorDetails
                       << " received: " << resultedInsert.ErrorDetails << ".";
            }
        }
        return ::testing::AssertionSuccess();
    }

    ::testing::AssertionResult productsAreEquivalent(
        const char* m_expr,
        const char* n_expr,
        const std::vector<ProductStatus>& expected,
        const std::vector<ProductStatus>& resulted)
    {
        std::stringstream s;
        s << m_expr << " and " << n_expr << " failed: ";

        if (expected.size() != resulted.size())
        {
            return ::testing::AssertionFailure() << s.str() << " products status differ. Expected " << expected.size()
                                                 << " products, but received: " << resulted.size() << ".";
        }

        for (size_t i = 0; i < expected.size(); i++)
        {
            auto& expectedInsert = expected[i];
            auto& resultedInsert = resulted[i];
            if (expectedInsert.RigidName != resultedInsert.RigidName)
            {
                return ::testing::AssertionFailure()
                       << s.str() << " rigidname differ. Expected " << expectedInsert.RigidName
                       << " received: " << resultedInsert.RigidName << ".";
            }
            if (expectedInsert.ProductName != resultedInsert.ProductName)
            {
                return ::testing::AssertionFailure()
                       << s.str() << " ProductName differ. Expected " << expectedInsert.ProductName
                       << " received: " << resultedInsert.ProductName << ".";
            }
            if (expectedInsert.DownloadedVersion != resultedInsert.DownloadedVersion)
            {
                return ::testing::AssertionFailure() << s.str() << " DownloadedVersion differ";
            }
        }
        return ::testing::AssertionSuccess();
    }

    ::testing::AssertionResult schedulerEventIsEquivalent(
        const char* m_expr,
        const char* n_expr,
        const UpdateEvent& expected,
        const UpdateEvent& resulted)
    {
        std::stringstream s;
        s << m_expr << " and " << n_expr << " failed: ";

        if (expected.IsRelevantToSend != resulted.IsRelevantToSend)
        {
            return ::testing::AssertionFailure()
                   << s.str() << "flag relevant to send differ. Expected: " << expected.IsRelevantToSend
                   << ". Received: " << resulted.IsRelevantToSend;
        }
        if (!expected.IsRelevantToSend)
        {
            // nothing else is important
            return ::testing::AssertionSuccess();
        }

        if (expected.MessageNumber != resulted.MessageNumber)
        {
            return ::testing::AssertionFailure()
                   << s.str() << " message number differ. Expected: " << expected.MessageNumber
                   << ". Received: " << resulted.MessageNumber;
        }

        if (expected.UpdateSource != resulted.UpdateSource)
        {
            return ::testing::AssertionFailure()
                   << s.str() << " update source differ. Expected: " << expected.UpdateSource
                   << ". Received: " << resulted.UpdateSource;
        }

        return insertMessagesAreEquivalent(m_expr, n_expr, expected.Messages, resulted.Messages);
    }

    ::testing::AssertionResult schedulerStatusIsEquivalent(
        const char* m_expr,
        const char* n_expr,
        const UpdateStatus& expected,
        const UpdateStatus& resulted)
    {
        std::stringstream s;
        s << m_expr << " and " << n_expr << " failed: ";


        if (expected.LastInstallStartedTime != resulted.LastInstallStartedTime)
        {
            return ::testing::AssertionFailure()
                   << s.str() << "LastInstallStartedTime differ. Expected: " << expected.LastInstallStartedTime
                   << ", received: " << resulted.LastInstallStartedTime;
        }

        if (expected.LastResult != resulted.LastResult)
        {
            return ::testing::AssertionFailure() << s.str() << "LastResult differ. Expected: " << expected.LastResult
                                                 << ", received: " << resulted.LastResult;
        }

        return productsAreEquivalent(m_expr, n_expr, expected.Subscriptions, resulted.Subscriptions);
    }

    UpdateEvent upgradeEvent()
    {
        UpdateEvent event;
        event.IsRelevantToSend = true;
        event.UpdateSource = SophosURL;
        event.MessageNumber = EventMessageNumber::SUCCESS;
        return event;
    }

    UpdateStatus upgradeStatus()
    {
        UpdateStatus status;
        status.LastInstallStartedTime = StartTimeTest;
        status.LastResult = 0;
        status.Products = { {"BaseRigidName", "BaseName", "0.5.0", "0.5.0"},
                            { "PluginRigidName", "PluginName", "0.5.0", "0.5.0"} };
        // Products
        //        std::string RigidName;
        //        std::string ProductName;
        //        std::string DownloadedVersion;
        //        std::string InstalledVersion;
        status.Subscriptions = { { "BaseRigidName", "BaseName", "0.5.0" },
                                 { "PluginRigidName", "PluginName", "0.5.0" } };
        return status;
    }

    std::vector<ReportCollectionResult::SignificantReportMark> shouldKeep(std::vector<bool> values)
    {
        std::vector<ReportCollectionResult::SignificantReportMark> marks;
        for (auto v : values)
        {
            marks.push_back(
                v ? ReportCollectionResult::SignificantReportMark::MustKeepReport
                  : ReportCollectionResult::SignificantReportMark::RedundantReport);
        }
        return marks;
    }

    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
};

TEST_F(TestDownloadReportAnalyser, ReportCollectionResultFromSingleSuccesfullUpgrade)
{
    DownloadReportsAnalyser::DownloadReportVector singleReport{ DownloadReportTestBuilder::goodReport() };
    ReportCollectionResult collectionResult = DownloadReportsAnalyser::processReports(singleReport);

    // send upgrade successful.
    UpdateEvent expectedEvent = upgradeEvent();
    UpdateStatus expectedStatus = upgradeStatus();

    EXPECT_PRED_FORMAT2(schedulerEventIsEquivalent, expectedEvent, collectionResult.SchedulerEvent);
    EXPECT_PRED_FORMAT2(schedulerStatusIsEquivalent, expectedStatus, collectionResult.SchedulerStatus);
    EXPECT_EQ(collectionResult.IndicesOfSignificantReports, shouldKeep({ true }));
}

TEST_F(TestDownloadReportAnalyser, OnTwoSuccessfullUpgradeKeepSecondOnly)
{
    DownloadReportsAnalyser::DownloadReportVector reports{ DownloadReportTestBuilder::goodReport(),
                                                           DownloadReportTestBuilder::goodReport() };

    ReportCollectionResult collectionResult = DownloadReportsAnalyser::processReports(reports);

    UpdateEvent expectedEvent = upgradeEvent();
    UpdateStatus expectedStatus = upgradeStatus();

    EXPECT_PRED_FORMAT2(schedulerEventIsEquivalent, expectedEvent, collectionResult.SchedulerEvent);
    EXPECT_PRED_FORMAT2(schedulerStatusIsEquivalent, expectedStatus, collectionResult.SchedulerStatus);
    EXPECT_EQ(collectionResult.IndicesOfSignificantReports, shouldKeep({ false, true }));
}

TEST_F(TestDownloadReportAnalyser, SecondEventNotUpgradeDoNotSendEvent)
{
    DownloadReportsAnalyser::DownloadReportVector reports{
        DownloadReportTestBuilder::goodReport(DownloadReportTestBuilder::UseTime::Previous),
        DownloadReportTestBuilder::goodReport(DownloadReportTestBuilder::UseTime::Later)
    };
    DownloadReportTestBuilder::setReportNoUpgrade(&reports[1]);

    reports[0].setProcessedReport(true);

    ReportCollectionResult collectionResult = DownloadReportsAnalyser::processReports(reports);

    UpdateEvent expectedEvent = upgradeEvent();
    expectedEvent.IsRelevantToSend = false;
    UpdateStatus expectedStatus = upgradeStatus();
    expectedStatus.LastInstallStartedTime = PreviousStartTime;

    EXPECT_PRED_FORMAT2(schedulerEventIsEquivalent, expectedEvent, collectionResult.SchedulerEvent);
    EXPECT_PRED_FORMAT2(schedulerStatusIsEquivalent, expectedStatus, collectionResult.SchedulerStatus);
    // keep both as the later is also the latest one.
    EXPECT_EQ(collectionResult.IndicesOfSignificantReports, shouldKeep({ true, true }));
}

TEST_F(TestDownloadReportAnalyser, ThirdAndSecondEventNotUpgradeDoNotSendEvent)
{
    DownloadReportsAnalyser::DownloadReportVector reports{
        DownloadReportTestBuilder::goodReport(DownloadReportTestBuilder::UseTime::Previous),
        DownloadReportTestBuilder::goodReport(DownloadReportTestBuilder::UseTime::Previous),
        DownloadReportTestBuilder::goodReport(DownloadReportTestBuilder::UseTime::Later)
    };
    DownloadReportTestBuilder::setReportNoUpgrade(&reports[1]);
    DownloadReportTestBuilder::setReportNoUpgrade(&reports[2]);

    reports[0].setProcessedReport(true);
    reports[1].setProcessedReport(true);

    ReportCollectionResult collectionResult = DownloadReportsAnalyser::processReports(reports);

    UpdateEvent expectedEvent = upgradeEvent();
    expectedEvent.IsRelevantToSend = false;
    UpdateStatus expectedStatus = upgradeStatus();
    expectedStatus.LastInstallStartedTime = PreviousStartTime;

    EXPECT_PRED_FORMAT2(schedulerEventIsEquivalent, expectedEvent, collectionResult.SchedulerEvent);
    EXPECT_PRED_FORMAT2(schedulerStatusIsEquivalent, expectedStatus, collectionResult.SchedulerStatus);
    // keep first as the most relevant (upgrade) and last as the most recent, second one not necessary
    EXPECT_EQ(collectionResult.IndicesOfSignificantReports, shouldKeep({ true, false, true }));
}

TEST_F(TestDownloadReportAnalyser, SingleFailureConnectionErrorAreReported)
{
    auto report = DownloadReportTestBuilder::badReport(
        DownloadReportTestBuilder::UseTime::Later, RepositoryStatus::CONNECTIONERROR, "failed2connect");

    DownloadReportsAnalyser::DownloadReportVector reports{ report };

    ReportCollectionResult collectionResult = DownloadReportsAnalyser::processReports(reports);

    UpdateEvent expectedEvent = upgradeEvent();
    expectedEvent.IsRelevantToSend = true;
    expectedEvent.MessageNumber = EventMessageNumber::CONNECTIONERROR;
    expectedEvent.Messages.emplace_back("", "failed2connect");
    UpdateStatus expectedStatus = upgradeStatus();
    expectedStatus.LastResult = 112;

    expectedStatus.Subscriptions.clear();

    EXPECT_PRED_FORMAT2(schedulerEventIsEquivalent, expectedEvent, collectionResult.SchedulerEvent);
    EXPECT_PRED_FORMAT2(schedulerStatusIsEquivalent, expectedStatus, collectionResult.SchedulerStatus);
    // keep first as the most relevant (upgrade) and last as the most recent, second one not necessary
    EXPECT_EQ(collectionResult.IndicesOfSignificantReports, shouldKeep({ true }));
}

TEST_F(TestDownloadReportAnalyser, SingleFailureInstallOfPluginErrorAreReported)
{
    auto report = DownloadReportTestBuilder::getPluginFailedToInstallReport(DownloadReportTestBuilder::UseTime::Later);

    DownloadReportsAnalyser::DownloadReportVector reports{ report };

    ReportCollectionResult collectionResult = DownloadReportsAnalyser::processReports(reports);

    UpdateEvent expectedEvent = upgradeEvent();
    expectedEvent.IsRelevantToSend = true;
    expectedEvent.MessageNumber = EventMessageNumber::INSTALLFAILED;
    expectedEvent.Messages.emplace_back("PluginName", "Plugin failed to install");
    UpdateStatus expectedStatus = upgradeStatus();
    expectedStatus.LastResult = 103;


    EXPECT_PRED_FORMAT2(schedulerEventIsEquivalent, expectedEvent, collectionResult.SchedulerEvent);
    EXPECT_PRED_FORMAT2(schedulerStatusIsEquivalent, expectedStatus, collectionResult.SchedulerStatus);
    // keep first as the most relevant (upgrade) and last as the most recent, second one not necessary
    EXPECT_EQ(collectionResult.IndicesOfSignificantReports, shouldKeep({ true }));
}

TEST_F(TestDownloadReportAnalyser, SuccessFollowedByInstallFailure)
{
    DownloadReportsAnalyser::DownloadReportVector reports{
        DownloadReportTestBuilder::goodReport(DownloadReportTestBuilder::UseTime::Previous),
        DownloadReportTestBuilder::getPluginFailedToInstallReport(DownloadReportTestBuilder::UseTime::Later)
    };

    ReportCollectionResult collectionResult = DownloadReportsAnalyser::processReports(reports);

    UpdateEvent expectedEvent = upgradeEvent();
    expectedEvent.IsRelevantToSend = true;
    expectedEvent.MessageNumber = EventMessageNumber::INSTALLFAILED;
    expectedEvent.Messages.emplace_back("PluginName", "Plugin failed to install");
    UpdateStatus expectedStatus = upgradeStatus();
    expectedStatus.LastResult = 103;
    expectedStatus.LastInstallStartedTime = PreviousStartTime;

    EXPECT_PRED_FORMAT2(schedulerEventIsEquivalent, expectedEvent, collectionResult.SchedulerEvent);
    EXPECT_PRED_FORMAT2(schedulerStatusIsEquivalent, expectedStatus, collectionResult.SchedulerStatus);
    // keep first as the most relevant (upgrade) and last as the most recent, second one not necessary
    EXPECT_EQ(collectionResult.IndicesOfSignificantReports, shouldKeep({ true, true }));
}

TEST_F(TestDownloadReportAnalyser, SuccessFollowedBy2Failures)
{
    DownloadReportsAnalyser::DownloadReportVector reports{
        DownloadReportTestBuilder::goodReport(DownloadReportTestBuilder::UseTime::PreviousPrevious),
        DownloadReportTestBuilder::getPluginFailedToInstallReport(DownloadReportTestBuilder::UseTime::Previous),
        DownloadReportTestBuilder::getPluginFailedToInstallReport(DownloadReportTestBuilder::UseTime::Later)
    };

    ReportCollectionResult collectionResult = DownloadReportsAnalyser::processReports(reports);

    UpdateEvent expectedEvent = upgradeEvent();
    expectedEvent.IsRelevantToSend = false; // not relevant to send as the information is the same as the previous one.
    expectedEvent.MessageNumber = EventMessageNumber::INSTALLFAILED;
    expectedEvent.Messages.emplace_back("PluginName", "Plugin failed to install");
    UpdateStatus expectedStatus = upgradeStatus();
    expectedStatus.LastResult = 103;
    expectedStatus.LastInstallStartedTime = PreviousPreviousStartTime;


    EXPECT_PRED_FORMAT2(schedulerEventIsEquivalent, expectedEvent, collectionResult.SchedulerEvent);
    EXPECT_PRED_FORMAT2(schedulerStatusIsEquivalent, expectedStatus, collectionResult.SchedulerStatus);
    // all as relevant. First one is the good one
    // second has the FirstFailedTime
    // third one is the latest.
    EXPECT_EQ(collectionResult.IndicesOfSignificantReports, shouldKeep({ true, true, true }));
}

TEST_F(TestDownloadReportAnalyser, SuccessFollowedBy2FailuresUsingFiles)
{
    auto report = DownloadReportTestBuilder::getPluginFailedToInstallReport(DownloadReportTestBuilder::UseTime::Later);
    std::string file1 =
        DownloadReportTestBuilder::goodReportString(DownloadReportTestBuilder::UseTime::PreviousPrevious);
    std::string file2 =
        DownloadReportTestBuilder::getPluginFailedToInstallReportString(DownloadReportTestBuilder::UseTime::Previous);
    std::string file3 =
        DownloadReportTestBuilder::getPluginFailedToInstallReportString(DownloadReportTestBuilder::UseTime::Later);
    // returning, on purpose, wrong order in the file system list files as it should not depend on that to list the
    // files in the chronological order.
    std::vector<std::string> files{ "update_report_1.json", "update_report_2.json", "update_report_3.json" };
    auto mockFileSystem = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*mockFileSystem, listFiles(_)).WillOnce(Return(files));
    EXPECT_CALL(*mockFileSystem, readFile("update_report_1.json")).WillOnce(Return(file1));
    EXPECT_CALL(*mockFileSystem, readFile("update_report_2.json")).WillOnce(Return(file2));
    EXPECT_CALL(*mockFileSystem, readFile("update_report_3.json")).WillOnce(Return(file3));

    EXPECT_CALL(*mockFileSystem, isFile("/opt/sophos-spl/base/update/var/updatescheduler/processedReports/update_report_1.json"))
        .WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, isFile("/opt/sophos-spl/base/update/var/updatescheduler/processedReports/update_report_2.json"))
        .WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, isFile("/opt/sophos-spl/base/update/var/updatescheduler/processedReports/update_report_3.json"))
        .WillOnce(Return(false));

    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    ReportAndFiles reportAndFiles = DownloadReportsAnalyser::processReports();
    ReportCollectionResult collectionResult = reportAndFiles.reportCollectionResult;

    std::vector<std::string> sortedOrder{ "update_report_1.json", "update_report_2.json", "update_report_3.json" };

    EXPECT_EQ(reportAndFiles.sortedFilePaths, sortedOrder);

    // copy from SuccessFollowedBy2Failures
    UpdateEvent expectedEvent = upgradeEvent();
    expectedEvent.IsRelevantToSend = false; // not relevant to send as the information is the same as the previous one.
    expectedEvent.MessageNumber = EventMessageNumber::INSTALLFAILED;
    expectedEvent.Messages.emplace_back("PluginName", "Plugin failed to install");
    UpdateStatus expectedStatus = upgradeStatus();
    expectedStatus.LastResult = EventMessageNumber::INSTALLFAILED;
    expectedStatus.LastInstallStartedTime = PreviousPreviousStartTime;

    EXPECT_PRED_FORMAT2(schedulerEventIsEquivalent, expectedEvent, collectionResult.SchedulerEvent);
    EXPECT_PRED_FORMAT2(schedulerStatusIsEquivalent, expectedStatus, collectionResult.SchedulerStatus);

    EXPECT_EQ(collectionResult.IndicesOfSignificantReports, shouldKeep({ true, true, true }));
}

TEST_F(TestDownloadReportAnalyser, ReportFileWithUnReadableDataLogsErrorAndFilterOutNonReports)
{
    testing::internal::CaptureStderr();

    auto report = DownloadReportTestBuilder::getPluginFailedToInstallReport(DownloadReportTestBuilder::UseTime::Later);
    std::string goodFile =
        DownloadReportTestBuilder::goodReportString(DownloadReportTestBuilder::UseTime::PreviousPrevious);
    std::string badFile = "<notjson?>thisIsInvalid>definitelynotjson<?!";
    // returning, on purpose, wrong order in the file system list files as it should not depend on that to list the
    // files in the chronological order.
    std::vector<std::string> files{
        "update_report_1.json", "update_report_2.json", "Config.json", "Update_Config.json", "random_unknown.json"
    };
    auto mockFileSystem = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*mockFileSystem, listFiles(_)).WillOnce(Return(files));
    EXPECT_CALL(*mockFileSystem, readFile("update_report_1.json")).WillOnce(Return(badFile));
    EXPECT_CALL(*mockFileSystem, readFile("update_report_2.json")).WillOnce(Return(goodFile));
    EXPECT_CALL(*mockFileSystem, isFile("/opt/sophos-spl/base/update/var/updatescheduler/processedReports/update_report_2.json"))
        .WillOnce(Return(false));

    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    ReportAndFiles reportAndFiles = DownloadReportsAnalyser::processReports();
    ReportCollectionResult collectionResult = reportAndFiles.reportCollectionResult;

    std::vector<std::string> sortedOrder{ "update_report_2.json" };

    EXPECT_EQ(reportAndFiles.sortedFilePaths, sortedOrder);

    // copy from Success
    UpdateEvent expectedEvent = upgradeEvent();
    expectedEvent.IsRelevantToSend = false; // not relevant to send as the information is the same as the previous one.
    expectedEvent.MessageNumber = EventMessageNumber::INSTALLFAILED;
    expectedEvent.Messages.emplace_back("PluginName", "Plugin failed to install");
    UpdateStatus expectedStatus = upgradeStatus();
    expectedStatus.LastResult = EventMessageNumber::INSTALLFAILED;
    expectedStatus.LastInstallStartedTime = PreviousPreviousStartTime;

    EXPECT_EQ(collectionResult.IndicesOfSignificantReports, shouldKeep({ true }));
    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Failed to process file: update_report_1.json"));
}

TEST_F(TestDownloadReportAnalyser, ProductsAreListedIfPossibleEvenOnConnectionError)
{
    DownloadReportsAnalyser::DownloadReportVector twoReports{ DownloadReportTestBuilder::goodReport(
                                                                  DownloadReportTestBuilder::UseTime::Previous),
                                                              DownloadReportTestBuilder::connectionError() };
    ReportCollectionResult collectionResult = DownloadReportsAnalyser::processReports(twoReports);

    UpdateStatus expectedStatus = upgradeStatus();
    expectedStatus.LastResult = 112;
    expectedStatus.LastInstallStartedTime = PreviousStartTime;

    EXPECT_PRED_FORMAT2(schedulerStatusIsEquivalent, expectedStatus, collectionResult.SchedulerStatus);
}

TEST_F(TestDownloadReportAnalyser, SerializationOfDownloadReports)
{
    auto report = DownloadReportTestBuilder::goodReport();
    auto serializedString = DownloadReportsAnalyser::DownloadReport::fromReport(report);
    auto deserializedReport = DownloadReportsAnalyser::DownloadReport::toReport(serializedString);
    auto re_serialized = DownloadReportsAnalyser::DownloadReport::fromReport(deserializedReport);
    EXPECT_EQ(serializedString, re_serialized);
    EXPECT_THAT(serializedString, ::testing::HasSubstr(SophosURL));
}

/**Acceptance criteria LINUXEP-6391 **/

TEST_F(TestDownloadReportAnalyser, FailedUpdateGeneratesCorrectStatusAndEvents)
{
    DownloadReportsAnalyser::DownloadReportVector reports{
        DownloadReportTestBuilder::goodReport(DownloadReportTestBuilder::UseTime::Previous),
        DownloadReportTestBuilder::getPluginFailedToInstallReport(DownloadReportTestBuilder::UseTime::Later)
    };

    ReportCollectionResult collectionResult = DownloadReportsAnalyser::processReports(reports);

    UpdateEvent expectedEvent = upgradeEvent();
    // event must be sent
    expectedEvent.IsRelevantToSend = true;
    expectedEvent.MessageNumber = EventMessageNumber::INSTALLFAILED;
    expectedEvent.Messages.emplace_back("PluginName", "Plugin failed to install");
    UpdateStatus expectedStatus = upgradeStatus();
    expectedStatus.LastResult = 103;
    expectedStatus.LastInstallStartedTime = PreviousStartTime;

    EXPECT_PRED_FORMAT2(schedulerEventIsEquivalent, expectedEvent, collectionResult.SchedulerEvent);
    EXPECT_PRED_FORMAT2(schedulerStatusIsEquivalent, expectedStatus, collectionResult.SchedulerStatus);
    // keep first as the most relevant (upgrade) and last as the most recent and the first failure
    EXPECT_EQ(collectionResult.IndicesOfSignificantReports, shouldKeep({ true, true }));
}

TEST_F(TestDownloadReportAnalyser, SuccessfulUpgradeSendEvents)
{
    DownloadReportsAnalyser::DownloadReportVector reports{
        DownloadReportTestBuilder::goodReport(DownloadReportTestBuilder::UseTime::Previous),
        DownloadReportTestBuilder::goodReport(DownloadReportTestBuilder::UseTime::Later)
    };

    ReportCollectionResult collectionResult = DownloadReportsAnalyser::processReports(reports);

    UpdateEvent expectedEvent = upgradeEvent();
    // event must be sent
    expectedEvent.IsRelevantToSend = true;
    expectedEvent.MessageNumber = EventMessageNumber::SUCCESS;
    UpdateStatus expectedStatus = upgradeStatus();
    expectedStatus.LastResult = 0;
    expectedStatus.LastInstallStartedTime = StartTimeTest;

    EXPECT_PRED_FORMAT2(schedulerEventIsEquivalent, expectedEvent, collectionResult.SchedulerEvent);
    EXPECT_PRED_FORMAT2(schedulerStatusIsEquivalent, expectedStatus, collectionResult.SchedulerStatus);
    // keep only the latter as the first is not necessary anymore.
    EXPECT_EQ(collectionResult.IndicesOfSignificantReports, shouldKeep({ false, true }));
}

TEST_F(TestDownloadReportAnalyser, UpgradeFollowedby2UpdateDoesNotSendEventWithNoCache)
{
    auto upgradeReport = DownloadReportTestBuilder::goodReport(DownloadReportTestBuilder::UseTime::PreviousPrevious);
    upgradeReport.setProcessedReport(true);
    // flag upgraded = false
    auto updateReport = DownloadReportTestBuilder::goodReport(DownloadReportTestBuilder::UseTime::Previous, false);
    updateReport.setProcessedReport(true);
    auto lastUpdateReport = DownloadReportTestBuilder::goodReport(DownloadReportTestBuilder::UseTime::Later, false);

    DownloadReportsAnalyser::DownloadReportVector reports{ upgradeReport, updateReport, lastUpdateReport };

    ReportCollectionResult collectionResult = DownloadReportsAnalyser::processReports(reports);

    UpdateEvent expectedEvent = upgradeEvent();
    // no event to be sent as update followed by upgrade
    expectedEvent.IsRelevantToSend = false;
    expectedEvent.MessageNumber = EventMessageNumber::SUCCESS;
    UpdateStatus expectedStatus = upgradeStatus();
    expectedStatus.LastResult = 0;
    expectedStatus.LastInstallStartedTime = PreviousPreviousStartTime;

    EXPECT_PRED_FORMAT2(schedulerEventIsEquivalent, expectedEvent, collectionResult.SchedulerEvent);
    EXPECT_PRED_FORMAT2(schedulerStatusIsEquivalent, expectedStatus, collectionResult.SchedulerStatus);
    // first is the upgrade and the later the last one the second one is not necessary
    EXPECT_EQ(collectionResult.IndicesOfSignificantReports, shouldKeep({ true, false, true }));
}

TEST_F(TestDownloadReportAnalyser, UpgradeFollowedby2UpdateDoesNotSendEventWithCache)
{
    auto upgradeReport =
        DownloadReportTestBuilder::goodReport(DownloadReportTestBuilder::UseTime::PreviousPrevious, true, "cache1");
    upgradeReport.setProcessedReport(true);
    // flag upgraded = false
    auto updateReport =
        DownloadReportTestBuilder::goodReport(DownloadReportTestBuilder::UseTime::Previous, false, "cache1");
    updateReport.setProcessedReport(true);
    auto lastUpdateReport =
        DownloadReportTestBuilder::goodReport(DownloadReportTestBuilder::UseTime::Later, false, "cache1");

    DownloadReportsAnalyser::DownloadReportVector reports{ upgradeReport, updateReport, lastUpdateReport };

    ReportCollectionResult collectionResult = DownloadReportsAnalyser::processReports(reports);

    UpdateEvent expectedEvent = upgradeEvent();
    // no event to be sent as update followed by upgrade
    expectedEvent.IsRelevantToSend = false;
    expectedEvent.MessageNumber = EventMessageNumber::SUCCESS;
    expectedEvent.UpdateSource = "cache1";
    UpdateStatus expectedStatus = upgradeStatus();
    expectedStatus.LastResult = 0;
    expectedStatus.LastInstallStartedTime = PreviousPreviousStartTime;

    EXPECT_PRED_FORMAT2(schedulerEventIsEquivalent, expectedEvent, collectionResult.SchedulerEvent);
    EXPECT_PRED_FORMAT2(schedulerStatusIsEquivalent, expectedStatus, collectionResult.SchedulerStatus);
    // first is the upgrade and the later the last one the second one is not necessary
    EXPECT_EQ(collectionResult.IndicesOfSignificantReports, shouldKeep({ true, false, true }));
}

TEST_F(TestDownloadReportAnalyser, UpgradeFollowedby2UpdateDoesNotSendEventWithCacheChanged)
{
    auto upgradeReport =
        DownloadReportTestBuilder::goodReport(DownloadReportTestBuilder::UseTime::PreviousPrevious, true, "cache1");
    // flag upgraded = false
    auto updateReport =
        DownloadReportTestBuilder::goodReport(DownloadReportTestBuilder::UseTime::Previous, false, "cache1");
    auto lastUpdateReport =
        DownloadReportTestBuilder::goodReport(DownloadReportTestBuilder::UseTime::Later, false, "cache2");

    DownloadReportsAnalyser::DownloadReportVector reports{ upgradeReport, updateReport, lastUpdateReport };

    ReportCollectionResult collectionResult = DownloadReportsAnalyser::processReports(reports);
    std::string d = DownloadReportsAnalyser::DownloadReport::fromReport(reports.at(0));
    UpdateEvent expectedEvent = upgradeEvent();
    // send event because cache changed
    expectedEvent.IsRelevantToSend = true;
    expectedEvent.MessageNumber = EventMessageNumber::SUCCESS;
    expectedEvent.UpdateSource = "cache2";
    UpdateStatus expectedStatus = upgradeStatus();
    expectedStatus.LastResult = 0;
    expectedStatus.LastInstallStartedTime = PreviousPreviousStartTime;

    EXPECT_PRED_FORMAT2(schedulerEventIsEquivalent, expectedEvent, collectionResult.SchedulerEvent);
    EXPECT_PRED_FORMAT2(schedulerStatusIsEquivalent, expectedStatus, collectionResult.SchedulerStatus);
    // first is the upgrade and the later the last one the second one is not necessary
    EXPECT_EQ(collectionResult.IndicesOfSignificantReports, shouldKeep({ true, false, true }));
}

TEST_F(TestDownloadReportAnalyser, exampleOfAnInstallFailedReport)
{
    static std::string reportExample{ R"sophos({
        "finishTime": "20180822 121220",
        "status": "INSTALLFAILED",
        "sulError": "",
        "products": [
        {
            "errorDescription": "",
            "rigidName": "ServerProtectionLinux-Base",
            "downloadVersion": "0.5.0",
            "productStatus": "UPTODATE",
            "productName": "ServerProtectionLinux-Base#0.5.0"
        },
        {
            "rigidName": "ServerProtectionLinux-Plugin",
            "errorDescription": "Failed to install",
            "downloadVersion": "0.5.0",
            "productStatus": "UPTODATE",
            "productName": "ServerProtectionLinux-Plugin#0.5"
        }
        ],
        "startTime": "20180822 121220",
        "errorDescription": "",
        "urlSource": "Sophos",
        "syncTime": "20180821 121220"
    })sophos" };
    DownloadReportsAnalyser::DownloadReport report = DownloadReportsAnalyser::DownloadReport::toReport(reportExample);
    DownloadReportsAnalyser::DownloadReportVector reports{ report };
    ReportCollectionResult collectionResult = DownloadReportsAnalyser::processReports(reports);

    UpdateEvent expectedEvent;
    // send event because cache changed
    expectedEvent.IsRelevantToSend = true;
    expectedEvent.MessageNumber = EventMessageNumber::INSTALLFAILED;
    expectedEvent.UpdateSource = "Sophos";

    // FIXME: LINUXEP-6473
    expectedEvent.Messages.emplace_back("ServerProtectionLinux-Plugin#0.5", "Failed to install");

    UpdateStatus expectedStatus;
    expectedStatus.LastResult = 103;
    expectedStatus.LastInstallStartedTime = "20180822 121220";
    expectedStatus.Subscriptions.emplace_back(
        "ServerProtectionLinux-Base", "ServerProtectionLinux-Base#0.5.0", "0.5.0");
    expectedStatus.Subscriptions.emplace_back(
        "ServerProtectionLinux-Plugin", "ServerProtectionLinux-Plugin#0.5", "0.5.0");

    EXPECT_PRED_FORMAT2(schedulerEventIsEquivalent, expectedEvent, collectionResult.SchedulerEvent);
    EXPECT_PRED_FORMAT2(schedulerStatusIsEquivalent, expectedStatus, collectionResult.SchedulerStatus);
    // first is the upgrade and the later the last one the second one is not necessary
    EXPECT_EQ(collectionResult.IndicesOfSignificantReports, shouldKeep({ true }));
}

TEST_F(TestDownloadReportAnalyser, exampleOf2SuccessiveUpdateReport)
{
    std::string firstReport{ R"sophos({ "finishTime": "20180821 121220",
"status": "SUCCESS",
"sulError": "",
"products": [ { "errorDescription": "",
"rigidName": "ServerProtectionLinux-Base",
"downloadVersion": "0.5.0",
"productStatus": "UPTODATE",
"productName": "ServerProtectionLinux-Base#0.5.0" },
{ "errorDescription": "",
"rigidName": "ServerProtectionLinux-Plugin",
"downloadVersion": "0.5.0",
"productStatus": "UPTODATE",
"productName": "ServerProtectionLinux-Plugin#0.5" } ],
"startTime": "20180821 121220",
"errorDescription": "",
"urlSource": "Sophos",
"syncTime": "20180821 121220" })sophos" };
    std::string lastReport{ R"sophos({
    "finishTime": "20180822 121220",
    "status": "SUCCESS",
    "sulError": "",
    "products": [
        {
            "errorDescription": "",
            "rigidName": "ServerProtectionLinux-Base",
            "downloadVersion": "0.5.0",
            "productStatus": "UPTODATE",
            "productName": "ServerProtectionLinux-Base#0.5.0"
        },
        {
            "errorDescription": "",
            "rigidName": "ServerProtectionLinux-Plugin",
            "downloadVersion": "0.5.0",
            "productStatus": "UPTODATE",
            "productName": "ServerProtectionLinux-Plugin#0.5"
        }
    ],
    "startTime": "20180822 121220",
    "errorDescription": "",
    "urlSource": "Sophos",
    "syncTime": "20180821 121220"
})sophos" };
    DownloadReport report1 = DownloadReport::toReport(firstReport);
    DownloadReport report2 = DownloadReport::toReport(lastReport);

    std::vector<DownloadReport> reports{ report1, report2 };
    ReportCollectionResult collectionResult = DownloadReportsAnalyser::processReports(reports);

    UpdateEvent expectedEvent;
    // send event because cache changed
    expectedEvent.IsRelevantToSend = false;
    expectedEvent.MessageNumber = EventMessageNumber::SUCCESS;
    expectedEvent.UpdateSource = "Sophos";

    UpdateStatus expectedStatus;
    expectedStatus.LastResult = 0;
    expectedStatus.LastInstallStartedTime = "20180822 121220";
    expectedStatus.Subscriptions.emplace_back(
        ProductStatus{ "ServerProtectionLinux-Base", "ServerProtectionLinux-Base#0.5.0", "0.5.0" });
    expectedStatus.Subscriptions.emplace_back(
        ProductStatus{ "ServerProtectionLinux-Plugin", "ServerProtectionLinux-Plugin#0.5", "0.5.0" });

    EXPECT_PRED_FORMAT2(schedulerEventIsEquivalent, expectedEvent, collectionResult.SchedulerEvent);
    EXPECT_PRED_FORMAT2(schedulerStatusIsEquivalent, expectedStatus, collectionResult.SchedulerStatus);
    // first is the upgrade and the later the last one the second one is not necessary
    EXPECT_EQ(collectionResult.IndicesOfSignificantReports, shouldKeep({ false, true }));
}

TEST_F(TestDownloadReportAnalyser, uninstalledProductsShouldGenerateEvent)
{
    std::string firstReport{
        R"sophos({ "startTime": "20190604 144145", "finishTime": "20190604 144155",
                "syncTime": "20190604 144155", "status": "SUCCESS", "sulError": "", "errorDescription": "",
                "urlSource": "https://localhost:1233",
                "products": [
                { "rigidName": "ServerProtectionLinux-Base", "productName": "Sophos Linux Protection ServerProtectionLinux-Base v0.5.0", "downloadVersion": "0.5.0", "errorDescription": "", "productStatus": "UPGRADED" },
                { "rigidName": "ServerProtectionLinux-Plugin-MDR", "productName": "Sophos Linux Protection ServerProtectionLinux-Plugin-MDR v0.5.0", "downloadVersion": "0.5.0", "errorDescription": "", "productStatus": "UPGRADED" } ] })sophos"
    };
    std::string lastReport{
        R"sophos({ "startTime": "20190604 144207", "finishTime": "20190604 144207", "syncTime": "20190604 144207", "status": "SUCCESS", "sulError": "", "errorDescription": "",
                "urlSource": "https://localhost:1233",
                "products": [
                {"rigidName": "ServerProtectionLinux-Base", "productName": "Sophos Linux Protection ServerProtectionLinux-Base v0.5.0", "downloadVersion": "0.5.0", "errorDescription": "", "productStatus": "UPTODATE" },
                { "rigidName": "ServerProtectionLinux-Plugin-MDR", "productName": "", "downloadVersion": "", "errorDescription": "", "productStatus": "UNINSTALLED" } ] })sophos"
    };
    DownloadReport report1 = DownloadReport::toReport(firstReport);
    DownloadReport report2 = DownloadReport::toReport(lastReport);

    std::vector<DownloadReport> reports{ report1, report2 };
    ReportCollectionResult collectionResult = DownloadReportsAnalyser::processReports(reports);

    UpdateEvent expectedEvent;
    // send event because cache changed
    expectedEvent.IsRelevantToSend = true;
    expectedEvent.MessageNumber = EventMessageNumber::SUCCESS;
    expectedEvent.UpdateSource = "https://localhost:1233";

    UpdateStatus expectedStatus;
    expectedStatus.LastResult = 0;
    expectedStatus.LastInstallStartedTime = "20190604 144145";
    expectedStatus.Subscriptions.emplace_back(ProductStatus{
        "ServerProtectionLinux-Base", "Sophos Linux Protection ServerProtectionLinux-Base v0.5.0", "0.5.0" });
    expectedStatus.Products.emplace_back(ProductStatus{
        "ServerProtectionLinux-Base", "Sophos Linux Protection ServerProtectionLinux-Base v0.5.0", "0.5.0" });

    // Plugin is not Reported, as it has been Uninstalled.

    EXPECT_PRED_FORMAT2(schedulerEventIsEquivalent, expectedEvent, collectionResult.SchedulerEvent);
    EXPECT_PRED_FORMAT2(schedulerStatusIsEquivalent, expectedStatus, collectionResult.SchedulerStatus);

    // the first and the last are significants. The first as it was the time base was installed.
    // The last because was when plugin was uninstalled.
    EXPECT_EQ(collectionResult.IndicesOfSignificantReports, shouldKeep({ true, true }));
}
