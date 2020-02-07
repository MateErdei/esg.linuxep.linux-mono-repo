/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include "manager/scheduler/ScanRunner.h"

#include <Common/UtilityImpl/StringUtils.h>

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
