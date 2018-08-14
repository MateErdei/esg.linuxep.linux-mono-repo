/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <SulDownloader/DownloadReport.h>
#include <UpdateScheduler/DownloadReportsAnalyser.h>

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
using namespace UpdateScheduler;
namespace  SulDownloader
{
    static const std::string StartTimeTest{"startTimeTest"};
    static const std::string FinishTimeTest{"finishTimeTest"};
    static const std::string PreviousStartTime{"previousStartTime"};
    static const std::string PreviousFinishTime{"previousFinishTime"};
    static const std::string PreviousPreviousStartTime{"PreviousPreviousStartTime"};
    static const std::string PreviousPreviousFinishTime{"PreviousPreviousFinishTime"};


    class DownloadReportTestBuilder
    {
    public:
        enum class UseTime{PreviousPrevious, Previous, Later};
        static SulDownloader::ProductReport upgradedBaseProduct()
        {
            SulDownloader::ProductReport report;
            report.name = "BaseName";
            report.rigidName = "BaseRigidName";
            report.downloadedVersion = "0.5.0";
            report.installedVersion = report.downloadedVersion;
            report.errorDescription.clear();
            report.productStatus = ProductReport::ProductStatus::Upgraded;
            return report;
        }

        static SulDownloader::ProductReport upgradedPlugin()
        {
            SulDownloader::ProductReport report;
            report.name = "PluginName";
            report.rigidName = "PluginRigidName";
            report.downloadedVersion = "0.5.0";
            report.installedVersion = report.downloadedVersion;
            report.errorDescription.clear();
            report.productStatus = ProductReport::ProductStatus::Upgraded;
            return report;
        }

        static std::vector<SulDownloader::ProductReport> goodProducts()
        {
            std::vector<SulDownloader::ProductReport> products;
            products.push_back(upgradedBaseProduct());
            products.push_back(upgradedPlugin());
            return products;
        }


        static SulDownloader::DownloadReport goodReport( UseTime useTime = UseTime::Later)
        {
            SulDownloader::DownloadReport report;
            report.m_status = WarehouseStatus::SUCCESS;
            report.m_description = "";
            report.m_sulError = "";
            switch (useTime)
            {
                case UseTime::Later:
                    report.m_startTime = StartTimeTest;
                    report.m_finishedTime = FinishTimeTest;
                    break;
                case UseTime::Previous:
                    report.m_startTime = PreviousStartTime;
                    report.m_finishedTime = PreviousFinishTime;
                    break;
                case UseTime::PreviousPrevious:
                    report.m_startTime = PreviousPreviousStartTime;
                    report.m_finishedTime = PreviousPreviousFinishTime;
                    break;

            }
            report.m_sync_time = report.m_finishedTime;
            report.m_productReport = goodProducts();
            return report;
        }

        static SulDownloader::DownloadReport badReport( UseTime useTime, WarehouseStatus status, std::string errorDescription)
        {
            SulDownloader::DownloadReport report;
            report.m_status = status;
            report.m_description = errorDescription;
            report.m_sulError = "";
            if ( useTime == UseTime::Later)
            {
                report.m_startTime = StartTimeTest;
                report.m_finishedTime = FinishTimeTest;
            }
            else
            {
                report.m_startTime = PreviousStartTime;
                report.m_finishedTime = PreviousFinishTime;
            }
            report.m_sync_time = report.m_finishedTime;

            if( status != SulDownloader::WarehouseStatus::CONNECTIONERROR)
            {
                report.m_productReport = goodProducts();
                for( auto & product: report.m_productReport)
                {
                    product.productStatus = SulDownloader::ProductReport::ProductStatus::SyncFailed;
                }

            }
            return report;
        }

        static SulDownloader::DownloadReport getPluginFailedToInstallReport(UseTime useTime)
        {
            auto report = badReport(useTime, WarehouseStatus::INSTALLFAILED, "");
            report.m_productReport[0].productStatus = ProductReport::ProductStatus::Upgraded;
            report.m_productReport[1].productStatus = ProductReport::ProductStatus::SyncFailed;
            report.m_productReport[1].errorDescription = "Plugin failed to install";
            return report;

        }


        static void setReportNoUpgrade(SulDownloader::DownloadReport * report)
        {
            for( auto & product: report->m_productReport)
            {
                product.productStatus = ProductReport::ProductStatus::UpToDate;
            }
        }


    };

}
using namespace SulDownloader;

class TestDownloadReportAnalyser : public  ::testing::Test
{
public:

    ::testing::AssertionResult insertMessagesAreEquivalent( const char* m_expr,
                                                           const char* n_expr,
                                                           const std::vector<MessageInsert> &  expected,
                                                           const std::vector<MessageInsert> & resulted)
    {
        std::stringstream s;
        s<< m_expr << " and " << n_expr << " failed: ";

        if( expected.size() != resulted.size())
        {
            return ::testing::AssertionFailure() << s.str() << " messages insert differ. Expected " << expected.size() << " messages, but received: " << resulted.size() << ".";
        }

        for (size_t i = 0; i< expected.size(); i++)
        {
            auto & expectedInsert = expected[i];
            auto & resultedInsert = resulted[i];
            if ( expectedInsert.PackageName != resultedInsert.PackageName)
            {
                return ::testing::AssertionFailure() << s.str() << " package name differ. Expected " << expectedInsert.PackageName << " received: " << resultedInsert.PackageName << ".";
            }
            if ( expectedInsert.ErrorDetails != resultedInsert.ErrorDetails)
            {
                return ::testing::AssertionFailure() << s.str() << " error details differ. Expected " << expectedInsert.ErrorDetails << " received: " << resultedInsert.ErrorDetails << ".";
            }

        }
        return ::testing::AssertionSuccess();

    }

    ::testing::AssertionResult productsAreEquivalent( const char* m_expr,
                                                            const char* n_expr,
                                                            const std::vector<ProductStatus> &  expected,
                                                            const std::vector<ProductStatus> & resulted)
    {
        std::stringstream s;
        s<< m_expr << " and " << n_expr << " failed: ";

        if( expected.size() != resulted.size())
        {
            return ::testing::AssertionFailure() << s.str() << " products status differ. Expected " << expected.size() << " products, but received: " << resulted.size() << ".";
        }

        for (size_t i = 0; i< expected.size(); i++)
        {
            auto & expectedInsert = expected[i];
            auto & resultedInsert = resulted[i];
            if ( expectedInsert.RigidName != resultedInsert.RigidName)
            {
                return ::testing::AssertionFailure() << s.str() << " rigidname differ. Expected " << expectedInsert.RigidName << " received: " << resultedInsert.RigidName << ".";
            }
            if ( expectedInsert.ProductName != resultedInsert.ProductName)
            {
                return ::testing::AssertionFailure() << s.str() << " ProductName differ. Expected " << expectedInsert.ProductName << " received: " << resultedInsert.ProductName << ".";
            }
            if ( expectedInsert.DownloadedVersion != resultedInsert.DownloadedVersion)
            {
                return ::testing::AssertionFailure() << s.str() << " DownloadedVersion differ";
            }
            if ( expectedInsert.InstalledVersion != resultedInsert.InstalledVersion)
            {
                return ::testing::AssertionFailure() << s.str() << " InstallVersion differ";
            }
        }
        return ::testing::AssertionSuccess();

    }


    ::testing::AssertionResult schedulerEventIsEquivalent( const char* m_expr,
                                                              const char* n_expr,
                                                              const UpdateEvent &  expected,
                                                              const UpdateEvent & resulted)
    {
        std::stringstream s;
        s<< m_expr << " and " << n_expr << " failed: ";

        if ( expected.IsRelevantToSend != resulted.IsRelevantToSend)
        {
            return ::testing::AssertionFailure() << s.str() << "flag relevant to send differ. Expected: " << expected.IsRelevantToSend << ". Received: " << resulted.IsRelevantToSend;
        }
        if ( !expected.IsRelevantToSend)
        {
            // nothing else is important
            return ::testing::AssertionSuccess();
        }

        if ( expected.MessageNumber != resulted.MessageNumber)
        {
            return ::testing::AssertionFailure() << s.str() << " message number differ. Expected: " << expected.MessageNumber << ". Received: " << resulted.MessageNumber;
        }

        if ( expected.UpdateSource != resulted.UpdateSource)
        {
            return ::testing::AssertionFailure() << s.str() << " update source differ. Expected: " << expected.UpdateSource << ". Received: " << resulted.UpdateSource;
        }

        return insertMessagesAreEquivalent( m_expr, n_expr, expected.Messages, resulted.Messages);
    }


    ::testing::AssertionResult schedulerStatusIsEquivalent( const char* m_expr,
                                                           const char* n_expr,
                                                           const UpdateStatus &  expected,
                                                           const UpdateStatus & resulted)
    {

        std::stringstream s;
        s<< m_expr << " and " << n_expr << " failed: ";

        // ignore LastBootTime

        if ( expected.LastStartTime != resulted.LastStartTime)
        {
            return ::testing::AssertionFailure() << s.str() << "LastStartTime differ. Expected: " << expected.LastStartTime << ", received: " << resulted.LastStartTime;
        }

        if ( expected.LastSyncTime != resulted.LastSyncTime)
        {
            return ::testing::AssertionFailure() << s.str() << "LastSyncTime differ. Expected: " << expected.LastSyncTime << ", received: " << resulted.LastSyncTime;
        }

        if ( expected.LastFinishdTime != resulted.LastFinishdTime)
        {
            return ::testing::AssertionFailure() << s.str() << " LastFinishdTime differ. Expected: " << expected.LastFinishdTime << ", received: " << resulted.LastFinishdTime;
        }

        if ( expected.LastInstallStartedTime != resulted.LastInstallStartedTime)
        {
            return ::testing::AssertionFailure() << s.str() << "LastInstallStartedTime differ. Expected: " << expected.LastInstallStartedTime << ", received: " << resulted.LastInstallStartedTime;
        }

        if ( expected.LastResult != resulted.LastResult)
        {
            return ::testing::AssertionFailure() << s.str() << "LastResult differ. Expected: " << expected.LastResult << ", received: " << resulted.LastResult;
        }

        if ( expected.FirstFailedTime != resulted.FirstFailedTime)
        {
            return ::testing::AssertionFailure() << s.str() << "FirstFailedTime differ. Expected: " << expected.FirstFailedTime << ", received: " << resulted.FirstFailedTime;
        }


        return productsAreEquivalent( m_expr, n_expr, expected.Products, resulted.Products);
    }

    UpdateEvent upgradeEvent()
    {
        UpdateEvent event;
        event.IsRelevantToSend = true;
        event.UpdateSource = "Sophos";
        event.MessageNumber = 0;
        return event;
    }

    UpdateStatus upgradeStatus()
    {
        UpdateStatus status;
        status.LastBootTime = StartTimeTest;
        status.LastStartTime = StartTimeTest;
        status.LastSyncTime = FinishTimeTest;
        status.LastFinishdTime = FinishTimeTest;
        status.LastInstallStartedTime = StartTimeTest;
        status.LastResult = 0;
        status.FirstFailedTime.clear();
        //Products
        //        std::string RigidName;
        //        std::string ProductName;
        //        std::string DownloadedVersion;
        //        std::string InstalledVersion;
        status.Products = {
                {"BaseRigidName", "BaseName", "0.5.0", "0.5.0"},
                {"PluginRigidName", "PluginName", "0.5.0", "0.5.0"}
        };
        return status;
    }

    std::vector<ReportCollectionResult::SignificantReportMark> shouldKeep(std::vector<bool> values)
    {
        std::vector<ReportCollectionResult::SignificantReportMark> marks;
        for( auto  v: values)
        {
            marks.push_back( v? ReportCollectionResult::SignificantReportMark::MustKeepReport : ReportCollectionResult::SignificantReportMark::RedundantReport);
        }
        return marks;
    }
};


TEST_F(TestDownloadReportAnalyser, ReportCollectionResultFromSingleSuccesfullUpgrade) // NOLINT
{
    std::vector<SulDownloader::DownloadReport> singleReport{SulDownloader::DownloadReportTestBuilder::goodReport()};
    ReportCollectionResult collectionResult =  DownloadReportsAnalyser::processReports(singleReport);

    // send upgrade successful.
    UpdateEvent expectedEvent = upgradeEvent();
    UpdateStatus expectedStatus = upgradeStatus();

    EXPECT_PRED_FORMAT2( schedulerEventIsEquivalent, expectedEvent, collectionResult.SchedulerEvent);
    EXPECT_PRED_FORMAT2( schedulerStatusIsEquivalent, expectedStatus, collectionResult.SchedulerStatus);
    EXPECT_EQ(collectionResult.IndicesOfSignificantReports, shouldKeep({true}));
}

TEST_F(TestDownloadReportAnalyser, OnTwoSuccessfullUpgradeKeepSecondOnly) // NOLINT
{
    std::vector<SulDownloader::DownloadReport> reports{
        DownloadReportTestBuilder::goodReport(),
        DownloadReportTestBuilder::goodReport()
    };

    ReportCollectionResult collectionResult =  DownloadReportsAnalyser::processReports(reports);


    UpdateEvent expectedEvent = upgradeEvent();
    UpdateStatus expectedStatus = upgradeStatus();

    EXPECT_PRED_FORMAT2( schedulerEventIsEquivalent, expectedEvent, collectionResult.SchedulerEvent);
    EXPECT_PRED_FORMAT2( schedulerStatusIsEquivalent, expectedStatus, collectionResult.SchedulerStatus);
    EXPECT_EQ(collectionResult.IndicesOfSignificantReports, shouldKeep({false, true}));
}


TEST_F(TestDownloadReportAnalyser, SecondEventNotUpgradeDoNotSendEvent) // NOLINT
{
    std::vector<SulDownloader::DownloadReport> reports{
            DownloadReportTestBuilder::goodReport(DownloadReportTestBuilder::UseTime::Previous),
            DownloadReportTestBuilder::goodReport(DownloadReportTestBuilder::UseTime::Later)
    };
    DownloadReportTestBuilder::setReportNoUpgrade(&reports[1]);

    ReportCollectionResult collectionResult =  DownloadReportsAnalyser::processReports(reports);


    UpdateEvent expectedEvent = upgradeEvent();
    expectedEvent.IsRelevantToSend = false;
    UpdateStatus expectedStatus = upgradeStatus();
    expectedStatus.LastInstallStartedTime = PreviousStartTime;

    EXPECT_PRED_FORMAT2( schedulerEventIsEquivalent, expectedEvent, collectionResult.SchedulerEvent);
    EXPECT_PRED_FORMAT2( schedulerStatusIsEquivalent, expectedStatus, collectionResult.SchedulerStatus);
    // keep both as the later is also the latest one.
    EXPECT_EQ(collectionResult.IndicesOfSignificantReports, shouldKeep({true, true}));
}


TEST_F(TestDownloadReportAnalyser, ThirdAndSecondEventNotUpgradeDoNotSendEvent) // NOLINT
{
    std::vector<SulDownloader::DownloadReport> reports{
            DownloadReportTestBuilder::goodReport(DownloadReportTestBuilder::UseTime::Previous),
            DownloadReportTestBuilder::goodReport(DownloadReportTestBuilder::UseTime::Previous),
            DownloadReportTestBuilder::goodReport(DownloadReportTestBuilder::UseTime::Later)
    };
    DownloadReportTestBuilder::setReportNoUpgrade(&reports[1]);
    DownloadReportTestBuilder::setReportNoUpgrade(&reports[2]);

    ReportCollectionResult collectionResult =  DownloadReportsAnalyser::processReports(reports);


    UpdateEvent expectedEvent = upgradeEvent();
    expectedEvent.IsRelevantToSend = false;
    UpdateStatus expectedStatus = upgradeStatus();
    expectedStatus.LastInstallStartedTime = PreviousStartTime;

    EXPECT_PRED_FORMAT2( schedulerEventIsEquivalent, expectedEvent, collectionResult.SchedulerEvent);
    EXPECT_PRED_FORMAT2( schedulerStatusIsEquivalent, expectedStatus, collectionResult.SchedulerStatus);
    // keep first as the most relevant (upgrade) and last as the most recent, second one not necessary
    EXPECT_EQ(collectionResult.IndicesOfSignificantReports, shouldKeep({true, false, true}));
}

TEST_F(TestDownloadReportAnalyser, SingleFailureConnectionErrorAreReported) // NOLINT
{
    auto report = DownloadReportTestBuilder::badReport(DownloadReportTestBuilder::UseTime::Later, SulDownloader::WarehouseStatus::CONNECTIONERROR, "failed2connect");

    std::vector<SulDownloader::DownloadReport> reports{
        report
    };

    ReportCollectionResult collectionResult =  DownloadReportsAnalyser::processReports(reports);

    UpdateEvent expectedEvent = upgradeEvent();
    expectedEvent.IsRelevantToSend = true;
    expectedEvent.MessageNumber = 112;
    expectedEvent.Messages.emplace_back("","failed2connect");
    UpdateStatus expectedStatus = upgradeStatus();
    expectedStatus.LastResult = 112;
    expectedStatus.LastSyncTime.clear();
    expectedStatus.LastInstallStartedTime.clear();
    expectedStatus.FirstFailedTime = StartTimeTest;
    expectedStatus.Products.clear();

    EXPECT_PRED_FORMAT2( schedulerEventIsEquivalent, expectedEvent, collectionResult.SchedulerEvent);
    EXPECT_PRED_FORMAT2( schedulerStatusIsEquivalent, expectedStatus, collectionResult.SchedulerStatus);
    // keep first as the most relevant (upgrade) and last as the most recent, second one not necessary
    EXPECT_EQ(collectionResult.IndicesOfSignificantReports, shouldKeep({true}));
}


TEST_F(TestDownloadReportAnalyser, SingleFailureInstallOfPluginErrorAreReported) // NOLINT
{
    auto report = DownloadReportTestBuilder::getPluginFailedToInstallReport(DownloadReportTestBuilder::UseTime::Later);

    std::vector<SulDownloader::DownloadReport> reports{
            report
    };

    ReportCollectionResult collectionResult =  DownloadReportsAnalyser::processReports(reports);

    UpdateEvent expectedEvent = upgradeEvent();
    expectedEvent.IsRelevantToSend = true;
    expectedEvent.MessageNumber = 103;
    expectedEvent.Messages.emplace_back("PluginName","Plugin failed to install");
    UpdateStatus expectedStatus = upgradeStatus();
    expectedStatus.LastResult = 103;
    expectedStatus.LastSyncTime.clear();
    expectedStatus.LastInstallStartedTime.clear();
    expectedStatus.FirstFailedTime = StartTimeTest;

    EXPECT_PRED_FORMAT2( schedulerEventIsEquivalent, expectedEvent, collectionResult.SchedulerEvent);
    EXPECT_PRED_FORMAT2( schedulerStatusIsEquivalent, expectedStatus, collectionResult.SchedulerStatus);
    // keep first as the most relevant (upgrade) and last as the most recent, second one not necessary
    EXPECT_EQ(collectionResult.IndicesOfSignificantReports, shouldKeep({true}));
}


TEST_F(TestDownloadReportAnalyser, SuccessFollowedByInstallFailure) // NOLINT
{
    std::vector<SulDownloader::DownloadReport> reports{
            DownloadReportTestBuilder::goodReport(DownloadReportTestBuilder::UseTime::Previous),
            DownloadReportTestBuilder::getPluginFailedToInstallReport(DownloadReportTestBuilder::UseTime::Later)
    };

    ReportCollectionResult collectionResult =  DownloadReportsAnalyser::processReports(reports);

    UpdateEvent expectedEvent = upgradeEvent();
    expectedEvent.IsRelevantToSend = true;
    expectedEvent.MessageNumber = 103;
    expectedEvent.Messages.emplace_back("PluginName","Plugin failed to install");
    UpdateStatus expectedStatus = upgradeStatus();
    expectedStatus.LastResult = 103;
    expectedStatus.LastSyncTime = PreviousFinishTime;
    expectedStatus.LastInstallStartedTime= PreviousStartTime;
    expectedStatus.FirstFailedTime = StartTimeTest;

    EXPECT_PRED_FORMAT2( schedulerEventIsEquivalent, expectedEvent, collectionResult.SchedulerEvent);
    EXPECT_PRED_FORMAT2( schedulerStatusIsEquivalent, expectedStatus, collectionResult.SchedulerStatus);
    // keep first as the most relevant (upgrade) and last as the most recent, second one not necessary
    EXPECT_EQ(collectionResult.IndicesOfSignificantReports, shouldKeep({true, true}));
}

TEST_F(TestDownloadReportAnalyser, SuccessFollowedBy2Failures) // NOLINT
{
    auto report = DownloadReportTestBuilder::getPluginFailedToInstallReport(DownloadReportTestBuilder::UseTime::Later);

    std::vector<SulDownloader::DownloadReport> reports{
            DownloadReportTestBuilder::goodReport(DownloadReportTestBuilder::UseTime::PreviousPrevious),
            DownloadReportTestBuilder::getPluginFailedToInstallReport(DownloadReportTestBuilder::UseTime::Previous),
            DownloadReportTestBuilder::getPluginFailedToInstallReport(DownloadReportTestBuilder::UseTime::Later)
    };

    ReportCollectionResult collectionResult =  DownloadReportsAnalyser::processReports(reports);

    UpdateEvent expectedEvent = upgradeEvent();
    expectedEvent.IsRelevantToSend = false; // not relevant to send as the information is the same as the previous one.
    expectedEvent.MessageNumber = 103;
    expectedEvent.Messages.emplace_back("PluginName","Plugin failed to install");
    UpdateStatus expectedStatus = upgradeStatus();
    expectedStatus.LastResult = 103;
    expectedStatus.LastSyncTime = PreviousPreviousFinishTime;
    expectedStatus.LastInstallStartedTime= PreviousPreviousStartTime;
    expectedStatus.FirstFailedTime = PreviousStartTime;

    EXPECT_PRED_FORMAT2( schedulerEventIsEquivalent, expectedEvent, collectionResult.SchedulerEvent);
    EXPECT_PRED_FORMAT2( schedulerStatusIsEquivalent, expectedStatus, collectionResult.SchedulerStatus);
    // all as relevant. First one is the good one
    // second has the FirstFailedTime
    // third one is the latest.
    EXPECT_EQ(collectionResult.IndicesOfSignificantReports, shouldKeep({true, true, true}));
}