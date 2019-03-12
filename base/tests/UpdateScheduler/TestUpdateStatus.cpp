/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "DownloadReportTestBuilder.h"

#include <Common/Logging/ConsoleLoggingSetup.h>
#include <UpdateSchedulerImpl/configModule/DownloadReportsAnalyser.h>
#include <UpdateSchedulerImpl/configModule/UpdateStatus.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include <tests/Common/UtilityImpl/MockFormattedTime.h>

using namespace UpdateSchedulerImpl::configModule;
using namespace SulDownloader;
using namespace SulDownloader::suldownloaderdata;
using namespace Common::UtilityImpl;
using namespace ::testing;

class TestSerializeStatus : public ::testing::Test
{
public:
    void SetUp() override { m_formattedTime = formattedTime(); }
    void TearDown() override {}

    std::unique_ptr<MockFormattedTime> formattedTime()
    {
        std::unique_ptr<MockFormattedTime> mockFormattedTime(new ::testing::StrictMock<MockFormattedTime>());
        EXPECT_CALL(*mockFormattedTime, bootTime()).WillOnce(Return("20180810 100000"));
        return std::move(mockFormattedTime);
    }

    void runTest(const std::string& expectedXML, const UpdateStatus& status);

    std::unique_ptr<MockFormattedTime> m_formattedTime;
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
};

static UpdateStatus getGoodStatus()
{
    DownloadReportsAnalyser::DownloadReportVector singleReport{ DownloadReportTestBuilder::goodReport() };
    ReportCollectionResult collectionResult = DownloadReportsAnalyser::processReports(singleReport);
    return collectionResult.SchedulerStatus;
}

static UpdateStatus getErrorStatus()
{
    DownloadReportsAnalyser::DownloadReportVector twoReports{ DownloadReportTestBuilder::goodReport(
                                                                  DownloadReportTestBuilder::UseTime::Previous),
                                                              DownloadReportTestBuilder::connectionError() };
    ReportCollectionResult collectionResult = DownloadReportsAnalyser::processReports(twoReports);
    return collectionResult.SchedulerStatus;
}

static boost::property_tree::ptree parseString(const std::string& input)
{
    namespace pt = boost::property_tree;
    std::istringstream i(input);
    pt::ptree tree;
    pt::xml_parser::read_xml(i, tree, pt::xml_parser::trim_whitespace | pt::xml_parser::no_comments);
    return tree;
}

void TestSerializeStatus::runTest(const std::string& expectedXML, const UpdateStatus& status)
{
    namespace pt = boost::property_tree;
    pt::ptree expectedTree = parseString(expectedXML);

    std::string actualOutput =
        SerializeUpdateStatus(status, "GivenRevId", "GivenVersion", "thisMachineID", *m_formattedTime);
    pt::ptree actualTree = parseString(actualOutput);

    if (actualTree != expectedTree)
    {
        std::cerr << "Incorrect actual XML: " << actualOutput << std::endl;
    }

    EXPECT_EQ(actualTree, expectedTree);
}

TEST_F(TestSerializeStatus, SuccessStatus) // NOLINT
{
    static const std::string normalStatusXML{ R"sophos(<?xml version="1.0" encoding="utf-8" ?>
<status xmlns="com.sophos\mansys\status" type="sau">
    <CompRes xmlns="com.sophos\msys\csc" Res="Same" RevID="GivenRevId" policyType="1" />
    <autoUpdate xmlns="http://www.sophos.com/xml/mansys/AutoUpdateStatus.xsd" version="GivenVersion">
        <lastBootTime>20180810 100000</lastBootTime>
        <lastStartedTime>20180812 10:00:00</lastStartedTime>
        <lastSyncTime>20180812 11:00:00</lastSyncTime>
        <lastInstallStartedTime>20180812 10:00:00</lastInstallStartedTime>
        <lastFinishedTime>20180812 11:00:00</lastFinishedTime>
        <lastResult>0</lastResult>
        <endpoint id="thisMachineID" />
    </autoUpdate>
    <subscriptions>
        <subscription rigidName="BaseRigidName" version="0.5.0" displayVersion="0.5.0" />
        <subscription rigidName="PluginRigidName" version="0.5.0" displayVersion="0.5.0" />
    </subscriptions>
</status>)sophos" };

    runTest(normalStatusXML, getGoodStatus());
}

TEST_F(TestSerializeStatus, FailedStatus) // NOLINT
{
    static const std::string errorReportedStatusXML{ R"sophos(<?xml version="1.0" encoding="utf-8" ?>
<status xmlns="com.sophos\mansys\status" type="sau">
    <CompRes xmlns="com.sophos\msys\csc" Res="Same" RevID="GivenRevId" policyType="1" />
    <autoUpdate xmlns="http://www.sophos.com/xml/mansys/AutoUpdateStatus.xsd" version="GivenVersion">
        <lastBootTime>20180810 100000</lastBootTime>
        <lastStartedTime>20180812 10:00:00</lastStartedTime>
        <lastSyncTime>20180811 11:00:00</lastSyncTime>
        <lastInstallStartedTime>20180811 10:00:00</lastInstallStartedTime>
        <lastFinishedTime>20180812 11:00:00</lastFinishedTime>
        <firstFailedTime>20180812 10:00:00</firstFailedTime>
        <lastResult>112</lastResult>
        <endpoint id="thisMachineID" />
    </autoUpdate>
    <subscriptions>
        <subscription rigidName="BaseRigidName" version="0.5.0" displayVersion="0.5.0" />
        <subscription rigidName="PluginRigidName" version="0.5.0" displayVersion="0.5.0" />
    </subscriptions>
</status>)sophos" };

    runTest(errorReportedStatusXML, getErrorStatus());
}
