// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "common/CentralEnums.h"

#include "Common/Helpers/LogInitializedTests.h"

using namespace common::CentralEnums;

namespace
{
    class TestCentralEnumsConvertThreatTypeStringToEnum : public LogOffInitializedTests
    {
    };
} // namespace

TEST_F(TestCentralEnumsConvertThreatTypeStringToEnum, ExpectedInputs)
{
    EXPECT_EQ(ConvertThreatTypeStringToEnum("virus"), ThreatType::virus);
    EXPECT_EQ(ConvertThreatTypeStringToEnum("trojan"), ThreatType::virus);
    EXPECT_EQ(ConvertThreatTypeStringToEnum("worm"), ThreatType::virus);
    EXPECT_EQ(ConvertThreatTypeStringToEnum("genotype"), ThreatType::virus);
    EXPECT_EQ(ConvertThreatTypeStringToEnum("nextgen"), ThreatType::virus);
    EXPECT_EQ(ConvertThreatTypeStringToEnum("PUA"), ThreatType::pua);
    EXPECT_EQ(ConvertThreatTypeStringToEnum("unknown"), ThreatType::unknown);
}

TEST_F(TestCentralEnumsConvertThreatTypeStringToEnum, UnexpectedInputs)
{
    EXPECT_THROW(ConvertThreatTypeStringToEnum("controlled app"), ThreatTypeConversionException);
    EXPECT_THROW(ConvertThreatTypeStringToEnum("undefined"), ThreatTypeConversionException);
    EXPECT_THROW(ConvertThreatTypeStringToEnum("Virus"), ThreatTypeConversionException);
    EXPECT_THROW(ConvertThreatTypeStringToEnum("pua"), ThreatTypeConversionException);
    EXPECT_THROW(ConvertThreatTypeStringToEnum("foo"), ThreatTypeConversionException);
    EXPECT_THROW(ConvertThreatTypeStringToEnum("?"), ThreatTypeConversionException);
}