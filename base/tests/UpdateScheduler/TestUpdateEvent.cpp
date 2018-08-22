/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gmock/gmock-matchers.h>
#include <UpdateSchedulerImpl/configModule/UpdateEvent.h>
#include <UpdateSchedulerImpl/configModule/DownloadReportsAnalyser.h>
#include <tests/Common/UtilityImpl/MockFormattedTime.h>
#include "DownloadReportTestBuilder.h"
#include "MockMapHostCacheId.h"
#include "gtest/gtest.h"


static std::string successEventXML{R"sophos(<?xml version="1.0"?>
<event xmlns="http://www.sophos.com/EE/AUEvent" type="sophos.mgt.entityAppEvent">
  <timestamp>20180816 083654</timestamp>
  <appInfo>
    <number>0</number>
  <updateSource>Sophos</updateSource>
  </appInfo>
  <entityInfo xmlns="http://www.sophos.com/EntityInfo">AGENT:WIN:1.0.0</entityInfo>
</event>)sophos"};

static std::string installFailedEventXML{R"sophos(<?xml version="1.0"?>
<event xmlns="http://www.sophos.com/EE/AUEvent" type="sophos.mgt.entityAppEvent">
  <timestamp>20180816 083654</timestamp>
  <appInfo>
    <number>103</number>
    <message>
      <message_inserts>
        <insert>BaseName</insert>
        <insert>CodeErrorA</insert>
        <insert>PluginName</insert>
        <insert>CodeErrorA</insert>
      </message_inserts>
    </message>
  <updateSource>Sophos</updateSource>
  </appInfo>
  <entityInfo xmlns="http://www.sophos.com/EntityInfo">AGENT:WIN:1.0.0</entityInfo>
</event>)sophos"};


static std::string installCaughtErrorEventXML{R"sophos(<?xml version="1.0"?>
<event xmlns="http://www.sophos.com/EE/AUEvent" type="sophos.mgt.entityAppEvent">
  <timestamp>20180816 083654</timestamp>
  <appInfo>
    <number>106</number>
    <message>
      <message_inserts>
        <insert>CodeErrorA</insert>
      </message_inserts>
    </message>
  <updateSource>Sophos</updateSource>
  </appInfo>
  <entityInfo xmlns="http://www.sophos.com/EntityInfo">AGENT:WIN:1.0.0</entityInfo>
</event>)sophos"};

static std::string downloadFailedEventXML{R"sophos(<?xml version="1.0"?>
<event xmlns="http://www.sophos.com/EE/AUEvent" type="sophos.mgt.entityAppEvent">
  <timestamp>20180816 083654</timestamp>
  <appInfo>
    <number>107</number>
    <message>
      <message_inserts>
        <insert>BaseName</insert>
        <insert>CodeErrorA</insert>
        <insert>PluginName</insert>
        <insert>CodeErrorA</insert>
      </message_inserts>
    </message>
  <updateSource>Sophos</updateSource>
  </appInfo>
  <entityInfo xmlns="http://www.sophos.com/EntityInfo">AGENT:WIN:1.0.0</entityInfo>
</event>)sophos"};

static std::string packageSourceMissingEventXML{R"sophos(<?xml version="1.0"?>
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
</event>)sophos"};

static std::string connectionErrorEventXML{R"sophos(<?xml version="1.0"?>
<event xmlns="http://www.sophos.com/EE/AUEvent" type="sophos.mgt.entityAppEvent">
  <timestamp>20180816 083654</timestamp>
  <appInfo>
    <number>112</number>
    <message>
      <message_inserts>
        <insert>CodeErrorA</insert>
      </message_inserts>
    </message>
  <updateSource>Sophos</updateSource>
  </appInfo>
  <entityInfo xmlns="http://www.sophos.com/EntityInfo">AGENT:WIN:1.0.0</entityInfo>
</event>)sophos"};

static std::string packagesSourceMissingEventXML{R"sophos(<?xml version="1.0"?>
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
</event>)sophos"};

using namespace UpdateSchedulerImpl::configModule;
using namespace SulDownloader;
using namespace Common::UtilityImpl;
using namespace ::testing;

UpdateEvent getEvent( SulDownloader::DownloadReport report )
{
    std::vector<SulDownloader::DownloadReport> singleReport{report};
    ReportCollectionResult collectionResult =  DownloadReportsAnalyser::processReports(singleReport);
    return  collectionResult.SchedulerEvent;
}



class TestSerializeEvent : public ::testing::Test
{
public:


    void SetUp() override
    {
        m_hostCacheId = hostCacheId();
        m_formattedTime = formattedTime();
    }
    void TearDown() override
    {

    }

    std::unique_ptr<MockMapHostCacheId> hostCacheId()
    {
        std::unique_ptr<MockMapHostCacheId> mockRevId( new ::testing::StrictMock<MockMapHostCacheId>());
        EXPECT_CALL(*mockRevId, cacheID(SophosURL)).WillOnce(Return("Sophos"));
        return std::move(mockRevId);
    }

    std::unique_ptr<MockFormattedTime> formattedTime()
    {
        std::unique_ptr<MockFormattedTime> mockFormattedTime( new ::testing::StrictMock<MockFormattedTime>());
        EXPECT_CALL(*mockFormattedTime, currentTime()).WillOnce(Return("20180816 083654"));
        return std::move(mockFormattedTime);

    }

    std::unique_ptr<MockMapHostCacheId> m_hostCacheId;
    std::unique_ptr<MockFormattedTime> m_formattedTime;
};



TEST_F(TestSerializeEvent, SuccessEvent) // NOLINT
{
    UpdateEvent event = getEvent(DownloadReportTestBuilder::goodReport() );
    EXPECT_EQ( serializeUpdateEvent(event,*m_hostCacheId, *m_formattedTime), successEventXML);
}

TEST_F(TestSerializeEvent, installFAiledTwoProducts) // NOLINT
{
    UpdateEvent event = getEvent(DownloadReportTestBuilder::installFailedTwoProducts() );
    EXPECT_EQ( serializeUpdateEvent(event,*m_hostCacheId, *m_formattedTime), installFailedEventXML);
}


TEST_F(TestSerializeEvent, installCaughtErrorEvent) // NOLINT
{
    UpdateEvent event = getEvent(DownloadReportTestBuilder::installCaughtError() );
    EXPECT_EQ( serializeUpdateEvent(event,*m_hostCacheId, *m_formattedTime), installCaughtErrorEventXML);
}


TEST_F(TestSerializeEvent, downloadFailedEvent) // NOLINT
{
    UpdateEvent event = getEvent(DownloadReportTestBuilder::downloadFailedError() );
    EXPECT_EQ( serializeUpdateEvent(event,*m_hostCacheId, *m_formattedTime), downloadFailedEventXML);
}

TEST_F(TestSerializeEvent, productMissing) // NOLINT
{
    UpdateEvent event = getEvent(DownloadReportTestBuilder::baseProductMissing() );
    EXPECT_EQ( serializeUpdateEvent(event,*m_hostCacheId, *m_formattedTime), packageSourceMissingEventXML);
}

TEST_F(TestSerializeEvent, productsMissing) // NOLINT
{
    UpdateEvent event = getEvent(DownloadReportTestBuilder::bothProductsMissing() );
    EXPECT_EQ( serializeUpdateEvent(event,*m_hostCacheId, *m_formattedTime), packagesSourceMissingEventXML);
}

TEST_F(TestSerializeEvent, connectionError) // NOLINT
{
    UpdateEvent event = getEvent(DownloadReportTestBuilder::connectionError() );
    EXPECT_EQ( serializeUpdateEvent(event,*m_hostCacheId, *m_formattedTime), connectionErrorEventXML);
}