// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "DownloadReportTestBuilder.h"

#include "Common/Logging/ConsoleLoggingSetup.h"
#include "UpdateSchedulerImpl/configModule/DownloadReportsAnalyser.h"
#include "UpdateSchedulerImpl/configModule/UpdateStatus.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "tests/Common/UtilityImpl/MockFormattedTime.h"

using namespace UpdateSchedulerImpl::configModule;
using namespace SulDownloader;
using namespace SulDownloader::suldownloaderdata;
using namespace Common::UtilityImpl;
using namespace ::testing;

namespace
{
    // Status expected to be the same on success and fail
    static const std::string goodStatusXML{ R"sophos(<?xml version="1.0" encoding="utf-8" ?>
<status xmlns="com.sophos\mansys\status" type="sau">
    <CompRes xmlns="com.sophos\msys\csc" Res="Same" RevID="GivenRevId" policyType="1" />
    <autoUpdate xmlns="http://www.sophos.com/xml/mansys/AutoUpdateStatus.xsd" version="GivenVersion">
        <endpoint id="thisMachineID" />
    <rebootState>
            <required>no</required>
    </rebootState>
    <downloadState>
            <state>good</state>
    </downloadState>
    <installState>
            <state>good</state>
            <lastGood>1970-01-01T03:25:45.000Z</lastGood>
    </installState>
    </autoUpdate>
    <subscriptions>
        <subscription rigidName="BaseRigidName" version="0.5.0" displayVersion="0.5.0" />
        <subscription rigidName="PluginRigidName" version="0.5.0" displayVersion="0.5.0" />
    </subscriptions>
    <products>
        <product rigidName="BaseRigidName" productName="BaseName" downloadedVersion="0.5.0" installedVersion="0.5.0" />
    </products>
    <Features>
    </Features>
</status>)sophos" };

    static const std::string badStatusXML{ R"sophos(<?xml version="1.0" encoding="utf-8" ?>
<status xmlns="com.sophos\mansys\status" type="sau">
    <CompRes xmlns="com.sophos\msys\csc" Res="Same" RevID="GivenRevId" policyType="1" />
    <autoUpdate xmlns="http://www.sophos.com/xml/mansys/AutoUpdateStatus.xsd" version="GivenVersion">
        <endpoint id="thisMachineID" />
    <rebootState>
            <required>no</required>
    </rebootState>
    <downloadState>
            <state>bad</state>
            <failedSince>1970-01-01T03:25:45.000Z</failedSince>
    </downloadState>
    <installState>
            <state>bad</state>
            <failedSince>1970-01-01T03:25:45.000Z</failedSince>
    </installState>
    </autoUpdate>
    <subscriptions>
        <subscription rigidName="BaseRigidName" version="0.5.0" displayVersion="0.5.0" />
        <subscription rigidName="PluginRigidName" version="0.5.0" displayVersion="0.5.0" />
    </subscriptions>
    <products>
        <product rigidName="BaseRigidName" productName="BaseName" downloadedVersion="0.5.0" installedVersion="0.5.0" />
    </products>
    <Features>
    </Features>
</status>)sophos" };
} // namespace

class TestSerializeStatus : public ::testing::Test
{
public:
    void SetUp() override { }
    void TearDown() override {}

    void runTest(const std::string& expectedXML,
                const UpdateStatus& status,
                const UpdateSchedulerImpl::StateData::StateMachineData& stateMachineData,
                const std::vector<std::string>& subscriptionsInPolicy );

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

void TestSerializeStatus::runTest(const std::string& expectedXML,
        const UpdateStatus& status,
        const UpdateSchedulerImpl::StateData::StateMachineData& stateMachineData,
                                  const std::vector<std::string>& subscriptionsInPolicy = {"BaseRigidName", "PluginRigidName"} )
{
    namespace pt = boost::property_tree;
    pt::ptree expectedTree = parseString(expectedXML);

    std::string actualOutput =
        SerializeUpdateStatus(status, "GivenRevId", "GivenVersion", "thisMachineID", subscriptionsInPolicy, {}, stateMachineData);
    pt::ptree actualTree = parseString(actualOutput);

    if (actualTree != expectedTree)
    {
        std::cerr << "Incorrect actual XML: " << actualOutput << std::endl;
    }

    EXPECT_EQ(actualTree, expectedTree);
}

TEST_F(TestSerializeStatus, SuccessStatusWithFeatures)
{
    static const std::string normalStatusWithFeaturesXML{ R"sophos(<?xml version="1.0" encoding="utf-8" ?>
<status xmlns="com.sophos\mansys\status" type="sau">
    <CompRes xmlns="com.sophos\msys\csc" Res="Same" RevID="GivenRevId" policyType="1" />
    <autoUpdate xmlns="http://www.sophos.com/xml/mansys/AutoUpdateStatus.xsd" version="GivenVersion">
        <endpoint id="thisMachineID" />
    <rebootState>
            <required>no</required>
    </rebootState>
    <downloadState>
            <state>good</state>
    </downloadState>
    <installState>
            <state>good</state>
            <lastGood>1970-01-01T03:25:45.000Z</lastGood>
    </installState>
    </autoUpdate>
    <subscriptions>
        <subscription rigidName="BaseRigidName" version="0.5.0" displayVersion="0.5.0" />
        <subscription rigidName="PluginRigidName" version="0.5.0" displayVersion="0.5.0" />
    </subscriptions>
    <products>
        <product rigidName="BaseRigidName" productName="BaseName" downloadedVersion="0.5.0" installedVersion="0.5.0" />
    </products>
    <Features>
        <Feature id="FeatureA"/>
    </Features>
</status>)sophos" };

    namespace pt = boost::property_tree;
    pt::ptree expectedTree = parseString(normalStatusWithFeaturesXML);

    UpdateSchedulerImpl::StateData::StateMachineData stateMachineData;
    stateMachineData.setDownloadState("0");
    stateMachineData.setDownloadFailedSinceTime("0");
    stateMachineData.setInstallState("0");
    stateMachineData.setLastGoodInstallTime("12345"); // will be converted to '1970-01-01T03:25:45.000Z'
    stateMachineData.setInstallFailedSinceTime("0");

    std::string actualOutput =
        SerializeUpdateStatus(getGoodStatus(),
                              "GivenRevId",
                              "GivenVersion",
                              "thisMachineID",
                              {"BaseRigidName", "PluginRigidName"},
                              {"FeatureA"},
                              stateMachineData);
    pt::ptree actualTree = parseString(actualOutput);

    if (actualTree != expectedTree)
    {
        std::cerr << "Incorrect actual XML: " << actualOutput << std::endl;
    }

    EXPECT_EQ(actualTree, expectedTree);
}

TEST_F(TestSerializeStatus, SuccessStatus)
{
    UpdateSchedulerImpl::StateData::StateMachineData stateMachineData;
    stateMachineData.setDownloadState("0");
    stateMachineData.setDownloadFailedSinceTime("0");
    stateMachineData.setInstallState("0");
    stateMachineData.setLastGoodInstallTime("12345"); // will be converted to '1970-01-01T03:25:45.000Z'
    stateMachineData.setInstallFailedSinceTime("0");

    runTest(goodStatusXML, getGoodStatus(), stateMachineData);
}

TEST_F(TestSerializeStatus, FailedStatus)
{
    UpdateSchedulerImpl::StateData::StateMachineData stateMachineData;
    stateMachineData.setDownloadState("1");
    stateMachineData.setDownloadFailedSinceTime("12345");
    stateMachineData.setInstallState("1");
    stateMachineData.setLastGoodInstallTime("0"); // will be converted to '1970-01-01T03:25:45.000Z'
    stateMachineData.setInstallFailedSinceTime("12345");

    runTest(badStatusXML, getErrorStatus(), stateMachineData);
}

TEST_F(TestSerializeStatus, SuccessStatusWithDefinedSubscriptions)
{
    static const std::string normalStatusWithDefinedSubscription{ R"sophos(<?xml version="1.0" encoding="utf-8" ?>
<status xmlns="com.sophos\mansys\status" type="sau">
    <CompRes xmlns="com.sophos\msys\csc" Res="Same" RevID="GivenRevId" policyType="1" />
    <autoUpdate xmlns="http://www.sophos.com/xml/mansys/AutoUpdateStatus.xsd" version="GivenVersion">
        <endpoint id="thisMachineID" />
        <rebootState>
            <required>no</required>
        </rebootState>
        <downloadState>
            <state>good</state>
        </downloadState>
        <installState>
            <state>good</state>
            <lastGood>1970-01-01T03:25:45.000Z</lastGood>
        </installState>
    </autoUpdate>
    <subscriptions>
        <subscription rigidName="ComponentSuite" version="v1" displayVersion="v1" />
    </subscriptions>
    <products>
        <product rigidName="BaseRigidName" productName="BaseName" downloadedVersion="0.5.0" installedVersion="0.5.0" />
    </products>
    <Features>
    </Features>
</status>)sophos" };
    DownloadReportsAnalyser::DownloadReportVector singleReport{
        DownloadReportTestBuilder::goodReportWithSubscriptions()
    };
    ReportCollectionResult collectionResult = DownloadReportsAnalyser::processReports(singleReport);

    UpdateSchedulerImpl::StateData::StateMachineData stateMachineData;
    stateMachineData.setDownloadState("0");
    stateMachineData.setDownloadFailedSinceTime("0");
    stateMachineData.setInstallState("0");
    stateMachineData.setLastGoodInstallTime("12345"); // will be converted to '1970-01-01T03:25:45.000Z'
    stateMachineData.setInstallFailedSinceTime("0");

    runTest(goodStatusXML, getGoodStatus(), stateMachineData);

    runTest(normalStatusWithDefinedSubscription, collectionResult.SchedulerStatus, stateMachineData, {"ComponentSuite"});
}

TEST_F(TestSerializeStatus, SuccessStatusWithCorrectInstalledVersionValues)
{
    static const std::string normalStatusWithDefinedSubscription{ R"sophos(<?xml version="1.0" encoding="utf-8" ?>
<status xmlns="com.sophos\mansys\status" type="sau">
    <CompRes xmlns="com.sophos\msys\csc" Res="Same" RevID="GivenRevId" policyType="1" />
    <autoUpdate xmlns="http://www.sophos.com/xml/mansys/AutoUpdateStatus.xsd" version="GivenVersion">
        <endpoint id="thisMachineID" />
        <rebootState>
            <required>no</required>
        </rebootState>
        <downloadState>
            <state>good</state>
        </downloadState>
        <installState>
            <state>good</state>
            <lastGood>1970-01-01T03:25:45.000Z</lastGood>
        </installState>
    </autoUpdate>
    <subscriptions>
        <subscription rigidName="BaseRigidName" version="v1" displayVersion="v1" />
    </subscriptions>
    <products>
        <product rigidName="BaseRigidName" productName="BaseName" downloadedVersion="0.5.0" installedVersion="0.5.0" />
    </products>
    <Features>
    </Features>
</status>)sophos" };
    DownloadReportsAnalyser::DownloadReportVector singleReport{
        DownloadReportTestBuilder::goodReportWithBaseRigidNameSubscriptions()
    };
    ReportCollectionResult collectionResult = DownloadReportsAnalyser::processReports(singleReport);

    UpdateSchedulerImpl::StateData::StateMachineData stateMachineData;
    stateMachineData.setDownloadState("0");
    stateMachineData.setDownloadFailedSinceTime("0");
    stateMachineData.setInstallState("0");
    stateMachineData.setLastGoodInstallTime("12345"); // will be converted to '1970-01-01T03:25:45.000Z'
    stateMachineData.setInstallFailedSinceTime("0");

    runTest(goodStatusXML, getGoodStatus(), stateMachineData);

    runTest(normalStatusWithDefinedSubscription, collectionResult.SchedulerStatus, stateMachineData, {"BaseRigidName"});
}

TEST_F(TestSerializeStatus, serializeStatusXmlCorrectlyFilteresOutRigidNamesNotInSubscriptionList)
{
    static const std::string expectedStatusXML{ R"sophos(<?xml version="1.0" encoding="utf-8" ?>
<status xmlns="com.sophos\mansys\status" type="sau">
    <CompRes xmlns="com.sophos\msys\csc" Res="Same" RevID="GivenRevId" policyType="1" />
    <autoUpdate xmlns="http://www.sophos.com/xml/mansys/AutoUpdateStatus.xsd" version="GivenVersion">
        <endpoint id="thisMachineID" />
    <rebootState>
            <required>no</required>
    </rebootState>
    <downloadState>
            <state>good</state>
    </downloadState>
    <installState>
            <state>good</state>
            <lastGood>1970-01-01T03:25:45.000Z</lastGood>
    </installState>
    </autoUpdate>
    <subscriptions>
        <subscription rigidName="BaseRigidName" version="0.5.0" displayVersion="0.5.0" />
    </subscriptions>
    <products>
        <product rigidName="BaseRigidName" productName="BaseName" downloadedVersion="0.5.0" installedVersion="0.5.0" />
    </products>
    <Features>
    </Features>
</status>)sophos" };

    UpdateSchedulerImpl::StateData::StateMachineData stateMachineData;
    stateMachineData.setDownloadState("0");
    stateMachineData.setDownloadFailedSinceTime("0");
    stateMachineData.setInstallState("0");
    stateMachineData.setLastGoodInstallTime("12345"); // will be converted to '1970-01-01T03:25:45.000Z'
    stateMachineData.setInstallFailedSinceTime("0");

    runTest(expectedStatusXML, getGoodStatus(), stateMachineData, {"BaseRigidName"});
}