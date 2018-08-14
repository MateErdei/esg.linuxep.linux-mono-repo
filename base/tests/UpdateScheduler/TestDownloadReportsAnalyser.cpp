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
    static const std::string StartTimeTest{"20180813T161535"};
    static const std::string FinishTimeTest{"20180813T162545"};
    static const std::string PreviousStartTime{"20180813T100000"};
    static const std::string PreviousFinishTime{"20180813T101020"};

    class DownloadReportTestBuilder
    {
    public:
        enum class UseTime{Previous, Later};
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
            report.m_productReport = goodProducts();
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
            return ::testing::AssertionFailure() << s.str() << "LastStartTime differ";
        }

        if ( expected.LastSyncTime != resulted.LastSyncTime)
        {
            return ::testing::AssertionFailure() << s.str() << "LastSyncTime differ";
        }

        if ( expected.LastFinishdTime != resulted.LastFinishdTime)
        {
            return ::testing::AssertionFailure() << s.str() << " LastFinishdTime differ";
        }

        if ( expected.LastInstallStartedTime != resulted.LastInstallStartedTime)
        {
            return ::testing::AssertionFailure() << s.str() << "LastInstallStartedTime differ";
        }

        if ( expected.LastResult != resulted.LastResult)
        {
            return ::testing::AssertionFailure() << s.str() << "LastResult differ";
        }

        if ( expected.FirstFailedTime != resulted.FirstFailedTime)
        {
            return ::testing::AssertionFailure() << s.str() << "FirstFailedTime differ";
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