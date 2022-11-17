// Copyright 2020-2022, Sophos Limited. All rights reserved.

#include "sophos_threat_detector/threat_scanner/SusiResultUtils.h"

#include <Common/Helpers/LogInitializedTests.h>
#include <gtest/gtest.h>
#include <log4cplus/loglevel.h>

#include <SusiTypes.h>

using namespace threat_scanner;
using namespace common::CentralEnums;

class TestSusiResultUtils : public LogOffInitializedTests
{
};

TEST_F(TestSusiResultUtils, TestSusiResultErrorToReadableErrorUnknown)
{
    auto loglevel = log4cplus::DEBUG_LOG_LEVEL;
    EXPECT_EQ(
        susiResultErrorToReadableError("test.file", static_cast<SusiResult>(17), loglevel),
        "Failed to scan test.file due to an unknown susi error [17]");

    EXPECT_EQ(loglevel, log4cplus::ERROR_LOG_LEVEL);
}

class ThreatScannerParameterizedTest
    : public ::testing::TestWithParam<std::tuple<std::string, std::string, log4cplus::LogLevel>>
{
};

INSTANTIATE_TEST_SUITE_P(
    TestSusiResultUtils,
    ThreatScannerParameterizedTest,
    ::testing::Values(
        // clang-format off
        std::make_tuple("encrypted", "Failed to scan test.file as it is password protected", log4cplus::WARN_LOG_LEVEL),
        std::make_tuple("corrupt", "Failed to scan test.file as it is corrupted", log4cplus::ERROR_LOG_LEVEL),
        std::make_tuple("unsupported", "Failed to scan test.file as it is not a supported file type", log4cplus::WARN_LOG_LEVEL),
        std::make_tuple("couldn't open", "Failed to scan test.file as it could not be opened", log4cplus::ERROR_LOG_LEVEL),
        std::make_tuple("recursion limit", "Failed to scan test.file as it is a Zip Bomb", log4cplus::ERROR_LOG_LEVEL),
        std::make_tuple("scan failed", "Failed to scan test.file due to a sweep failure", log4cplus::ERROR_LOG_LEVEL),
        std::make_tuple("unexpected (0x80040231)", "Failed to scan test.file [unexpected (0x80040231)]", log4cplus::ERROR_LOG_LEVEL)
        // clang-format on
        ));

TEST_P(ThreatScannerParameterizedTest, susiErrorToReadableError)
{
    auto loglevel = log4cplus::DEBUG_LOG_LEVEL;
    EXPECT_EQ(susiErrorToReadableError("test.file", std::get<0>(GetParam()), loglevel), std::get<1>(GetParam()));
    EXPECT_EQ(loglevel, std::get<2>(GetParam()));
}

class SusiResultErrorToReadableErrorParameterized
    : public ::testing::TestWithParam<std::tuple<SusiResult, std::string, log4cplus::LogLevel>>
{
};

INSTANTIATE_TEST_SUITE_P(
    TestSusiResultUtils,
    SusiResultErrorToReadableErrorParameterized,
    ::testing::Values(
        // clang-format off
        std::make_tuple(SUSI_E_INTERNAL, "Failed to scan test.file due to an internal susi error", log4cplus::ERROR_LOG_LEVEL),
        std::make_tuple(SUSI_E_INVALIDARG, "Failed to scan test.file due to a susi invalid argument error", log4cplus::ERROR_LOG_LEVEL),
        std::make_tuple(SUSI_E_OUTOFMEMORY, "Failed to scan test.file due to a susi out of memory error", log4cplus::ERROR_LOG_LEVEL),
        std::make_tuple(SUSI_E_OUTOFDISK, "Failed to scan test.file due to a susi out of disk error", log4cplus::ERROR_LOG_LEVEL),
        std::make_tuple(SUSI_E_CORRUPTDATA, "Failed to scan test.file as it is corrupted", log4cplus::ERROR_LOG_LEVEL),
        std::make_tuple(SUSI_E_INVALIDCONFIG, "Failed to scan test.file due to an invalid susi config", log4cplus::ERROR_LOG_LEVEL),
        std::make_tuple(SUSI_E_INVALIDTEMPDIR, "Failed to scan test.file due to an invalid susi temporary directory", log4cplus::ERROR_LOG_LEVEL),
        std::make_tuple(SUSI_E_INITIALIZING, "Failed to scan test.file due to a failure to initialize susi", log4cplus::ERROR_LOG_LEVEL),
        std::make_tuple(SUSI_E_NOTINITIALIZED, "Failed to scan test.file due to susi not being initialized", log4cplus::ERROR_LOG_LEVEL),
        std::make_tuple(SUSI_E_ALREADYINITIALIZED, "Failed to scan test.file due to susi already being initialized", log4cplus::ERROR_LOG_LEVEL),
        std::make_tuple(SUSI_E_SCANFAILURE, "Failed to scan test.file due to a generic scan failure", log4cplus::ERROR_LOG_LEVEL),
        std::make_tuple(SUSI_E_SCANABORTED, "Failed to scan test.file as the scan was aborted", log4cplus::ERROR_LOG_LEVEL),
        std::make_tuple(SUSI_E_FILEOPEN, "Failed to scan test.file as it could not be opened", log4cplus::ERROR_LOG_LEVEL),
        std::make_tuple(SUSI_E_FILEREAD, "Failed to scan test.file as it could not be read", log4cplus::ERROR_LOG_LEVEL),
        std::make_tuple(SUSI_E_FILEENCRYPTED, "Failed to scan test.file as it is password protected", log4cplus::WARN_LOG_LEVEL),
        std::make_tuple(SUSI_E_FILEMULTIVOLUME, "Failed to scan test.file due to a multi-volume error", log4cplus::ERROR_LOG_LEVEL),
        std::make_tuple(SUSI_E_FILECORRUPT, "Failed to scan test.file as it is corrupted", log4cplus::ERROR_LOG_LEVEL),
        std::make_tuple(SUSI_E_CALLBACK, "Failed to scan test.file due to a callback error", log4cplus::ERROR_LOG_LEVEL),
        std::make_tuple(SUSI_E_BAD_JSON, "Failed to scan test.file due to a failure to parse scan result", log4cplus::ERROR_LOG_LEVEL),
        std::make_tuple(SUSI_E_MANIFEST, "Failed to scan test.file due to a bad susi manifest", log4cplus::ERROR_LOG_LEVEL)
        // clang-format on
        ));

TEST_P(SusiResultErrorToReadableErrorParameterized, susiResultErrorToReadableError)
{
    auto loglevel = log4cplus::DEBUG_LOG_LEVEL;
    EXPECT_EQ(susiResultErrorToReadableError("test.file", std::get<0>(GetParam()), loglevel), std::get<1>(GetParam()));
    EXPECT_EQ(loglevel, std::get<2>(GetParam()));
}

TEST_F(TestSusiResultUtils, convertSusiThreatTypeExpectedInputs)
{
    EXPECT_EQ(convertSusiThreatType("virus"), ThreatType::virus);
    EXPECT_EQ(convertSusiThreatType("trojan"), ThreatType::virus);
    EXPECT_EQ(convertSusiThreatType("worm"), ThreatType::virus);
    EXPECT_EQ(convertSusiThreatType("genotype"), ThreatType::virus);
    EXPECT_EQ(convertSusiThreatType("nextgen"), ThreatType::virus);
    EXPECT_EQ(convertSusiThreatType("PUA"), ThreatType::pua);
    EXPECT_EQ(convertSusiThreatType("controlled app"), ThreatType::unknown);
    EXPECT_EQ(convertSusiThreatType("undefined"), ThreatType::unknown);
    EXPECT_EQ(convertSusiThreatType("unknown"), ThreatType::unknown);
}

TEST_F(TestSusiResultUtils, convertSusiThreatTypeUnexpectedInputs)
{
    EXPECT_EQ(convertSusiThreatType("Virus"), ThreatType::unknown);
    EXPECT_EQ(convertSusiThreatType("pua"), ThreatType::unknown);
    EXPECT_EQ(convertSusiThreatType("foo"), ThreatType::unknown);
    EXPECT_EQ(convertSusiThreatType("?"), ThreatType::unknown);
}

TEST_F(TestSusiResultUtils, getReportSource)
{
    EXPECT_EQ(getReportSource("EICAR-AV-Test"), ReportSource::vdl);
    EXPECT_EQ(getReportSource("ML/PE-A"), ReportSource::ml);
    EXPECT_EQ(getReportSource("Mal/Generic-R"), ReportSource::vdl);
    EXPECT_EQ(getReportSource("Generic Reputation PUA"), ReportSource::vdl);
    EXPECT_EQ(getReportSource("Troj/TestSFS-D"), ReportSource::vdl);
    EXPECT_EQ(getReportSource("Vir/evil"), ReportSource::vdl);
    EXPECT_EQ(getReportSource("App/dodgy"), ReportSource::vdl);
    EXPECT_EQ(getReportSource("Worm/evil"), ReportSource::vdl);
    EXPECT_EQ(getReportSource("NextGen/evil"), ReportSource::vdl);
}
