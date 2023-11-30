// Copyright 2023 Sophos Limited. All rights reserved.

#include "pluginimpl/NTPStatus.h"

#include "Common/XmlUtilities/AttributesMap.h"

#include "base/tests/Common/Helpers/MemoryAppender.h"

#include "pluginimpl/config.h"

#include <gtest/gtest.h>

namespace
{
    class TestNTPStatus : public MemoryAppenderUsingTests
    {
    public:
        TestNTPStatus() : MemoryAppenderUsingTests(PLUGIN_NAME)
        {}
    };
}

using namespace Plugin;

TEST_F(TestNTPStatus, fixedValues)
{
    NTPStatus status{"", false};
    auto xml = status.xml();
    auto attrmap = Common::XmlUtilities::parseXml(xml); // throws for invalid XML
    auto elements = attrmap.lookupMultiple("status/isolation");
    ASSERT_EQ(elements.size(), 1);
    auto isolation = elements.at(0);
    EXPECT_EQ(isolation.value("self"), "false");

    elements = attrmap.lookupMultiple("status/meta");
    ASSERT_EQ(elements.size(), 1);
    auto meta = elements.at(0);
    EXPECT_EQ(meta.value("softwareVersion"), PLUGIN_VERSION);
}

TEST_F(TestNTPStatus, emptyRevId)
{
    NTPStatus status{"", false};
    auto xml = status.xml();
    auto attrmap = Common::XmlUtilities::parseXml(xml); // throws for invalid XML
    auto elements = attrmap.lookupMultiple("status/csc:CompRes");
    ASSERT_EQ(elements.size(), 1);
    auto comp = elements.at(0);
    EXPECT_EQ(comp.value("RevID"), "");
    EXPECT_EQ(comp.value("Res"), "NoRef");
}

TEST_F(TestNTPStatus, nonEmptyRevId)
{
    NTPStatus status{"ARevId", false};
    auto xml = status.xml();
    auto attrmap = Common::XmlUtilities::parseXml(xml); // throws for invalid XML
    auto elements = attrmap.lookupMultiple("status/csc:CompRes");
    ASSERT_EQ(elements.size(), 1);
    auto comp = elements.at(0);
    EXPECT_EQ(comp.value("RevID"), "ARevId");
    EXPECT_EQ(comp.value("Res"), "Same");
}

TEST_F(TestNTPStatus, adminIsolationFalse)
{
    NTPStatus status{"", false};
    auto xml = status.xml();
    auto attrmap = Common::XmlUtilities::parseXml(xml); // throws for invalid XML
    auto isolationElements = attrmap.lookupMultiple("status/isolation");
    ASSERT_EQ(isolationElements.size(), 1);
    auto isolation = isolationElements.at(0);
    EXPECT_EQ(isolation.value("admin"), "false");
}

TEST_F(TestNTPStatus, adminIsolationTrue)
{
    NTPStatus status{"", true};
    auto xml = status.xml();
    auto attrmap = Common::XmlUtilities::parseXml(xml); // throws for invalid XML
    auto isolationElements = attrmap.lookupMultiple("status/isolation");
    ASSERT_EQ(isolationElements.size(), 1);
    auto isolation = isolationElements.at(0);
    EXPECT_EQ(isolation.value("admin"), "true");
}

TEST_F(TestNTPStatus, withTimestampHasTimestamp)
{
    NTPStatus status{"", true};
    auto xml = status.xml();
    auto attrmap = Common::XmlUtilities::parseXml(xml); // throws for invalid XML
    auto elements = attrmap.lookupMultiple("status/meta");
    ASSERT_EQ(elements.size(), 1);
    auto meta = elements.at(0);
    EXPECT_NE(meta.value("timestamp"), "");
}

TEST_F(TestNTPStatus, withoutTimestampHasNoTimestamp)
{
    NTPStatus status{"", true};
    auto xml = status.xmlWithoutTimestamp();
    auto attrmap = Common::XmlUtilities::parseXml(xml); // throws for invalid XML
    auto elements = attrmap.lookupMultiple("status/meta");
    ASSERT_EQ(elements.size(), 1);
    auto meta = elements.at(0);
    EXPECT_EQ(meta.value("timestamp"), "");
}
