/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "DownloadReportTestBuilder.h"
#include "MockMapHostCacheId.h"

#include <UpdateSchedulerImpl/configModule/DownloadReportsAnalyser.h>
#include <UpdateSchedulerImpl/configModule/UpdateEvent.h>
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

UpdateEvent getEvent(const DownloadReportsAnalyser::DownloadReport& report)
{
    DownloadReportsAnalyser::DownloadReportVector singleReport{ report };
    ReportCollectionResult collectionResult = DownloadReportsAnalyser::processReports(singleReport);
    return collectionResult.SchedulerEvent;
}

class TestSerializeEvent : public ::testing::Test
{
public:
    void SetUp() override
    {
        m_hostCacheId = hostCacheId();
        m_formattedTime = formattedTime();
    }
    void TearDown() override {}

    std::unique_ptr<MockMapHostCacheId> hostCacheId()
    {
        std::unique_ptr<MockMapHostCacheId> mockRevId(new ::testing::StrictMock<MockMapHostCacheId>());
        EXPECT_CALL(*mockRevId, cacheID(SophosURL)).WillOnce(Return("Sophos"));
        return mockRevId;
    }

    std::unique_ptr<MockFormattedTime> formattedTime()
    {
        std::unique_ptr<MockFormattedTime> mockFormattedTime(new ::testing::StrictMock<MockFormattedTime>());
        EXPECT_CALL(*mockFormattedTime, currentTime()).WillOnce(Return("20180816 083654"));
        return mockFormattedTime;
    }

    void runTest(const UpdateEvent& event, const std::string& expectedXML);

    std::unique_ptr<MockMapHostCacheId> m_hostCacheId;
    std::unique_ptr<MockFormattedTime> m_formattedTime;
};

static boost::property_tree::ptree parseString(const std::string& input)
{
    namespace pt = boost::property_tree;
    std::istringstream i(input);
    pt::ptree tree;
    pt::xml_parser::read_xml(i, tree, pt::xml_parser::trim_whitespace | pt::xml_parser::no_comments);
    return tree;
}

void TestSerializeEvent::runTest(const UpdateEvent& event, const std::string& expectedXML)
{
    auto expectedTree = parseString(expectedXML);
    auto actualXml = serializeUpdateEvent(event, *m_hostCacheId, *m_formattedTime);
    auto actualTree = parseString(actualXml);

    if (expectedTree != actualTree)
    {
        std::cerr << "Incorrect actual XML: " << actualXml << std::endl;
    }

    EXPECT_EQ(actualTree, expectedTree);
}

TEST_F(TestSerializeEvent, SuccessEvent) // NOLINT
{
    static const std::string successEventXML{ R"sophos(<?xml version="1.0"?>
<event xmlns="http://www.sophos.com/EE/AUEvent" type="sophos.mgt.entityAppEvent">
  <timestamp>20180816 083654</timestamp>
  <appInfo>
    <number>0</number>
  <updateSource>Sophos</updateSource>
  </appInfo>
  <entityInfo xmlns="http://www.sophos.com/EntityInfo">AGENT:WIN:1.0.0</entityInfo>
</event>)sophos" };

    UpdateEvent event = getEvent(DownloadReportTestBuilder::goodReport());
    runTest(event, successEventXML);
}

TEST_F(TestSerializeEvent, installFAiledTwoProducts) // NOLINT
{
    static const std::string installFailedEventXML{ R"sophos(<?xml version="1.0"?>
<event xmlns="http://www.sophos.com/EE/AUEvent" type="sophos.mgt.entityAppEvent">
  <timestamp>20180816 083654</timestamp>
  <appInfo>
    <number>103</number>
    <message>
      <message_inserts>
        <insert>BaseName</insert>
        <insert>PluginName</insert>
      </message_inserts>
    </message>
  <updateSource>Sophos</updateSource>
  </appInfo>
  <entityInfo xmlns="http://www.sophos.com/EntityInfo">AGENT:WIN:1.0.0</entityInfo>
</event>)sophos" };

    UpdateEvent event = getEvent(DownloadReportTestBuilder::installFailedTwoProducts());
    runTest(event, installFailedEventXML);
}

TEST_F(TestSerializeEvent, installCaughtErrorEvent) // NOLINT
{
    static const std::string installCaughtErrorEventXML{ R"sophos(<?xml version="1.0"?>
<event xmlns="http://www.sophos.com/EE/AUEvent" type="sophos.mgt.entityAppEvent">
  <timestamp>20180816 083654</timestamp>
  <appInfo>
    <number>106</number>
    <message>
      <message_inserts>
      </message_inserts>
    </message>
  <updateSource>Sophos</updateSource>
  </appInfo>
  <entityInfo xmlns="http://www.sophos.com/EntityInfo">AGENT:WIN:1.0.0</entityInfo>
</event>)sophos" };

    UpdateEvent event = getEvent(DownloadReportTestBuilder::installCaughtError());
    runTest(event, installCaughtErrorEventXML);
}

TEST_F(TestSerializeEvent, downloadFailedEvent) // NOLINT
{
    static const std::string downloadFailedEventXML{ R"sophos(<?xml version="1.0"?>
<event xmlns="http://www.sophos.com/EE/AUEvent" type="sophos.mgt.entityAppEvent">
  <timestamp>20180816 083654</timestamp>
  <appInfo>
    <number>107</number>
    <message>
      <message_inserts>
        <insert>BaseName</insert>
        <insert>PluginName</insert>
      </message_inserts>
    </message>
  <updateSource>Sophos</updateSource>
  </appInfo>
  <entityInfo xmlns="http://www.sophos.com/EntityInfo">AGENT:WIN:1.0.0</entityInfo>
</event>)sophos" };

    UpdateEvent event = getEvent(DownloadReportTestBuilder::downloadFailedError());
    runTest(event, downloadFailedEventXML);
}

TEST_F(TestSerializeEvent, productMissing) // NOLINT
{
    static const std::string packageSourceMissingEventXML{ R"sophos(<?xml version="1.0"?>
<event xmlns="http://www.sophos.com/EE/AUEvent" type="sophos.mgt.entityAppEvent">
  <timestamp>20180816 083654</timestamp>
  <appInfo>
    <number>111</number>
    <message>
      <message_inserts>
        <insert>BaseName</insert>
      </message_inserts>
    </message>
  <updateSource>Sophos</updateSource>
  </appInfo>
  <entityInfo xmlns="http://www.sophos.com/EntityInfo">AGENT:WIN:1.0.0</entityInfo>
</event>)sophos" };

    UpdateEvent event = getEvent(DownloadReportTestBuilder::baseProductMissing());
    runTest(event, packageSourceMissingEventXML);
}

TEST_F(TestSerializeEvent, productsMissing) // NOLINT
{
    static const std::string packagesSourceMissingEventXML{ R"sophos(<?xml version="1.0"?>
<event xmlns="http://www.sophos.com/EE/AUEvent" type="sophos.mgt.entityAppEvent">
  <timestamp>20180816 083654</timestamp>
  <appInfo>
    <number>113</number>
    <message>
      <message_inserts>
        <insert>BaseName</insert>
        <insert>PluginName</insert>
      </message_inserts>
    </message>
  <updateSource>Sophos</updateSource>
  </appInfo>
  <entityInfo xmlns="http://www.sophos.com/EntityInfo">AGENT:WIN:1.0.0</entityInfo>
</event>)sophos" };

    UpdateEvent event = getEvent(DownloadReportTestBuilder::bothProductsMissing());
    runTest(event, packagesSourceMissingEventXML);
}

TEST_F(TestSerializeEvent, connectionError) // NOLINT
{
    static const std::string connectionErrorEventXML{ R"sophos(<?xml version="1.0"?>
<event xmlns="http://www.sophos.com/EE/AUEvent" type="sophos.mgt.entityAppEvent">
  <timestamp>20180816 083654</timestamp>
  <appInfo>
    <number>112</number>
    <message>
      <message_inserts>
      </message_inserts>
    </message>
  <updateSource>Sophos</updateSource>
  </appInfo>
  <entityInfo xmlns="http://www.sophos.com/EntityInfo">AGENT:WIN:1.0.0</entityInfo>
</event>)sophos" };

    UpdateEvent event = getEvent(DownloadReportTestBuilder::connectionError());
    runTest(event, connectionErrorEventXML);
}