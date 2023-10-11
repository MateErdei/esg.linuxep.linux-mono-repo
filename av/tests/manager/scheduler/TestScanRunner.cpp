/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include "manager/scheduler/ScanRunner.h"
#include "datatypes/Time.h"

#include <Common/UtilityImpl/StringUtils.h>

#include <chrono>
#include <thread>

using namespace manager::scheduler;

TEST(TestScanRunner, construction) // NOLINT
{
    class FakeScanCompletion : public IScanComplete
    {
    public:
        void processScanComplete(std::string& scanCompletedXml) override
        {
            m_xml = scanCompletedXml;
        }
        std::string m_xml;
    };

    FakeScanCompletion scanCompletion;
    ScanRunner test("TestScan", "", scanCompletion);
    static_cast<void>(test);
}

TEST(TestScanRunner, testCompletionXmlGenerationDoesntContainTemplateName) //NOLINT
{
    std::string xml = generateScanCompleteXml("FOO");
    EXPECT_THAT(xml, Not(::testing::HasSubstr("<scanName>@@SCANNAME@@</scanName>")));
}

TEST(TestScanRunner, testCompletionXmlGenerationDoesContainTimestamp) // NOLINT
{
    std::string xml = generateScanCompleteXml("FOO");
    EXPECT_THAT(xml, Not(::testing::HasSubstr("<timestamp></timestamp>")));
    EXPECT_THAT(xml, ::testing::HasSubstr("<timestamp>"));
    EXPECT_THAT(xml, ::testing::HasSubstr("</timestamp>"));
}

TEST(TestScanRunner, testCompletionXmlGenerationChangesTimestamp) // NOLINT
{
    std::string xml1 = generateScanCompleteXml("FOO");
    std::this_thread::sleep_for(std::chrono::milliseconds(1001));
    std::string xml2 = generateScanCompleteXml("FOO");
    EXPECT_NE(xml1, xml2);
}

TEST(TestScanRunner, testTimestampGeneration) // NOLINT
{
    std::string timestamp1 = datatypes::Time::currentToCentralTime();
    EXPECT_THAT(timestamp1, ::testing::MatchesRegex("[0-9]{8} [0-9]{6}"));

    std::string timestamp2 = datatypes::Time::currentToCentralTime();
    EXPECT_THAT(timestamp2, ::testing::MatchesRegex("[0-9]{8} [0-9]{6}"));
}

