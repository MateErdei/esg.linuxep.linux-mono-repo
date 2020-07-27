/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include <datatypes/Print.h>
#include "manager/scheduler/DaySet.h"

using namespace manager::scheduler;

namespace {
    class TestDaySet : public ::testing::Test {
    public:
        Common::XmlUtilities::AttributesMap m_days = Common::XmlUtilities::parseXml(
                R"MULTILINE(<?xml version="1.0" encoding="utf-8"?>
                <daySet>
                    <day>monday</day>
                    <day>tuesday</day>
                    <day>wednesday</day>
                    <day>thursday</day>
                    <day>friday</day>
                    <day>saturday</day>
                    <day>sunday</day>
                </daySet>)MULTILINE");
    };
}


TEST_F(TestDaySet, construction) // NOLINT
{
    DaySet set(m_days, "daySet/day");
}
