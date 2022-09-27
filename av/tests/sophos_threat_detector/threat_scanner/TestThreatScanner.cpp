// Copyright 2020-2022, Sophos Limited.  All rights reserved.

# define TEST_PUBLIC public

#include "MockSusiWrapper.h"
#include "MockSusiWrapperFactory.h"

#include "../../common/MemoryAppender.h"
#include "../../common/Common.h"
#include "../../common/WaitForEvent.h"
#include "sophos_threat_detector/threat_scanner/SusiScanner.h"
#include "sophos_threat_detector/threat_scanner/ThrowIfNotOk.h"
#include "unixsocket/threatReporterSocket/ThreatReporterServerSocket.h"

#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/UtilityImpl/StringUtils.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;
using namespace threat_scanner;

static const std::string cleanSusiResponseStr =
    "{\n"
    "    \"time\":\"/...\",\n"
    "    \"results\":\n"
    "    [\n"
    "    ]\n"
    "}";

static const std::string singleDetectionSusiResponseStr =
    "{\n"
    "    \"time\":\"/...\",\n"
    "    \"results\":\n"
    "    [\n"
    "        {\n"
    "            \"base64path\":\"L3RtcC9laWNhci5pc28vMS9kaXJlY3Rvcnkvc3ViZGlyL2VpY2FyLmNvbQ==\",\n"
    "            \"detections\":[\n"
    "               {\n"
    "                   \"threatName\":\"MAL/malware-A\",\n"
    "                   \"threatType\":\"malware/trojan/PUA/...\"\n"
    "               }\n"
    "            ],\n"
    "           \"path\":\"/tmp/eicar.txt\",\n"
    "           \"sha256\":\"...\",\n"
    "           \"TFTclassification\": {\n"
    "                \"fileType\":\"...\",\n"
    "                \"fileTypeDescription\":\"...\",\n"
    "                \"typeId\":\"...\"\n"
    "            },\n"
    "           \"submitToAnalysis\": {\n"
    "                \"identity\":\"...\",\n"
    "                \"revision\":\"...\"\n"
    "            }\n"
    "        }\n"
    "    ]\n"
    "}";

static const std::string multipleDetectionSusiResponseStr =
    "{\n"
    "    \"time\":\"/...\",\n"
    "    \"results\":\n"
    "    [\n"
    "        {\n"
    "            \"path\":\"/tmp/eicar.txt\",\n"
    "            \"sha256\":\"...\",\n"
    "            \"base64path\":\"L3RtcC9laWNhci5pc28vMS9kaXJlY3Rvcnkvc3ViZGlyL2VpY2FyLmNvbQ==\",\n"
    "            \"detections\":[\n"
    "               {\n"
    "                   \"threatName\":\"MAL/malware-A\",\n"
    "                   \"threatType\":\"malware/trojan/PUA/...\"\n"
    "               }\n"
    "            ],\n"
    "            \"TFTclassification\": {\n"
    "                \"fileType\":\"...\",\n"
    "                \"fileTypeDescription\":\"...\",\n"
    "                \"typeId\":\"...\"\n"
    "            },\n"
    "            \"submitToAnalysis\": {\n"
    "                \"identity\":\"...\",\n"
    "                \"revision\":\"...\"\n"
    "            }\n"
    "        },\n"
    "        {\n"
    "            \"path\":\"/tmp/eicar.com\",\n"
    "            \"sha256\":\"...\",\n"
    "            \"base64path\":\"L3RtcC9laWNhci5pc28vMS9kaXJlY3Rvcnkvc3ViZGlyL2VpY2FyLmNvbQ==\",\n"
    "            \"detections\":[\n"
    "               {\n"
    "                   \"threatName\":\"EICAR-AV-Test\",\n"
    "                   \"threatType\":\"virus\"\n"
    "               }\n"
    "            ],\n"
    "            \"TFTclassification\": {\n"
    "                \"fileType\":\"...\",\n"
    "                \"fileTypeDescription\":\"...\",\n"
    "                \"typeId\":\"...\"\n"
    "            },\n"
    "            \"submitToAnalysis\": {\n"
    "                \"identity\":\"...\",\n"
    "                \"revision\":\"...\"\n"
    "            }\n"
    "        }\n"
    "    ]\n"
    "}";

static const std::string errorSusiResponseStr =
    "{\n"
    "    \"time\":\"/...\",\n"
    "    \"results\":\n"
    "    [\n"
    "        {\n"
    "            \"path\":\"/tmp/eicar.txt\",\n"
    "            \"sha256\":\"...\",\n"
    "            \"base64path\":\"L3RtcC9laWNhci5pc28vMS9kaXJlY3Rvcnkvc3ViZGlyL2VpY2FyLmNvbQ==\",\n"
    "            \"error\":\"encrypted\",\n"
    "            \"TFTclassification\": {\n"
    "                \"fileType\":\"...\",\n"
    "                \"fileTypeDescription\":\"...\",\n"
    "                \"typeId\":\"...\"\n"
    "            },\n"
    "            \"submitToAnalysis\": {\n"
    "                \"identity\":\"...\",\n"
    "                \"revision\":\"...\"\n"
    "            }\n"
    "        }\n"
    "    ]\n"
    "}";

namespace
{

    class TestThreatScanner : public MemoryAppenderUsingTests
    {
    public:
        TestThreatScanner() : MemoryAppenderUsingTests("ThreatScanner") {}

    };


    class MockIThreatReporter : public threat_scanner::IThreatReporter
    {
    public:
        /**
         *
        virtual void sendThreatReport(
            const std::string& threatPath,
            const std::string& threatName,
            int64_t scanType,
            const std::string& userID,
            std::time_t detectionTimeStamp) = 0;
         */
        MOCK_METHOD(void, sendThreatReport, (const std::string& threatPath,
                                             const std::string& threatName,
                                             const std::string& sha256,
                                             int64_t scanType,
                                             const std::string& userID,
                                             std::time_t detectionTimeStamp));
    };

    class MockShutdownTimer : public threat_scanner::IScanNotification
    {
    public:
        MOCK_METHOD(void, reset, ());
        MOCK_METHOD(time_t, timeout, ());
    };
}

TEST_F(TestThreatScanner, test_SusiScannerConstruction)
{
    setupFakeSophosThreatDetectorConfig();

    fs::path libraryPath = pluginInstall() / "chroot/susi/distribution_version";

    static const std::string scannerInfo = R"("scanner": {
        "signatureBased": {
            "fileTypeCategories": {
                "archive": false,
                "selfExtractor": true,
                "executable": true,
                "office": true,
                "adobe": true,
                "android": true,
                "internet": true,
                "webArchive": false,
                "webEncoding": true,
                "media": true,
                "macintosh": true,
                "discImage": false
            },
            "scanControl": {
                "trueFileTypeDetection": false,
                "puaDetection": false,
                "archiveRecursionDepth": 16,
                "stopOnArchiveBombs": true,
                "submitToAnalysis": false
            }
        }
    })";

    std::string scannerConfig = Common::UtilityImpl::StringUtils::orderedStringReplace(
        "{@@SCANNER_CONFIG@@}", {{"@@SCANNER_CONFIG@@", scannerInfo} });

    auto susiWrapper = std::make_shared<MockSusiWrapper>(scannerConfig);
    std::shared_ptr<MockSusiWrapperFactory> susiWrapperFactory = std::make_shared<MockSusiWrapperFactory>();

    EXPECT_CALL(*susiWrapperFactory, createSusiWrapper(scannerConfig)).WillOnce(Return(susiWrapper));
    threat_scanner::SusiScanner susiScanner(susiWrapperFactory, false, false, nullptr, nullptr);
}

TEST_F(TestThreatScanner, test_SusiScannerConstructionWithScanArchives)
{
    setupFakeSophosThreatDetectorConfig();

    fs::path libraryPath = pluginInstall() / "chroot/susi/distribution_version";

    static const std::string scannerInfo = R"("scanner": {
        "signatureBased": {
            "fileTypeCategories": {
                "archive": true,
                "selfExtractor": true,
                "executable": true,
                "office": true,
                "adobe": true,
                "android": true,
                "internet": true,
                "webArchive": true,
                "webEncoding": true,
                "media": true,
                "macintosh": true,
                "discImage": false
            },
            "scanControl": {
                "trueFileTypeDetection": false,
                "puaDetection": false,
                "archiveRecursionDepth": 16,
                "stopOnArchiveBombs": true,
                "submitToAnalysis": false
            }
        }
    })";

    auto scannerConfig = Common::UtilityImpl::StringUtils::orderedStringReplace(
        "{@@SCANNER_CONFIG@@}", {{"@@SCANNER_CONFIG@@", scannerInfo} });


    auto susiWrapper = std::make_shared<MockSusiWrapper>(scannerConfig);
    std::shared_ptr<MockSusiWrapperFactory> susiWrapperFactory = std::make_shared<MockSusiWrapperFactory>();

    EXPECT_CALL(*susiWrapperFactory, createSusiWrapper(scannerConfig)).WillOnce(Return(susiWrapper));
    threat_scanner::SusiScanner susiScanner(susiWrapperFactory, true, false, nullptr, nullptr);
}

TEST_F(TestThreatScanner, test_SusiScannerConstructionWithScanImages)
{
    setupFakeSophosThreatDetectorConfig();

    fs::path libraryPath = pluginInstall() / "chroot/susi/distribution_version";

    static const std::string scannerInfo = R"("scanner": {
        "signatureBased": {
            "fileTypeCategories": {
                "archive": false,
                "selfExtractor": true,
                "executable": true,
                "office": true,
                "adobe": true,
                "android": true,
                "internet": true,
                "webArchive": false,
                "webEncoding": true,
                "media": true,
                "macintosh": true,
                "discImage": true
            },
            "scanControl": {
                "trueFileTypeDetection": false,
                "puaDetection": false,
                "archiveRecursionDepth": 16,
                "stopOnArchiveBombs": true,
                "submitToAnalysis": false
            }
        }
    })";

    auto scannerConfig = Common::UtilityImpl::StringUtils::orderedStringReplace(
            "{@@SCANNER_CONFIG@@}", {{"@@SCANNER_CONFIG@@", scannerInfo} });


    auto susiWrapper = std::make_shared<MockSusiWrapper>(scannerConfig);
    std::shared_ptr<MockSusiWrapperFactory> susiWrapperFactory = std::make_shared<MockSusiWrapperFactory>();

    EXPECT_CALL(*susiWrapperFactory, createSusiWrapper(scannerConfig)).WillOnce(Return(susiWrapper));
    threat_scanner::SusiScanner susiScanner(susiWrapperFactory, false, true, nullptr, nullptr);
}

TEST_F(TestThreatScanner, test_SusiScanner_scanFile_clean)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    setupFakeSophosThreatDetectorConfig();

    auto susiWrapper = std::make_shared<MockSusiWrapper>("");
    auto susiWrapperFactory = std::make_shared<MockSusiWrapperFactory>();
    auto mock_timer = std::make_shared<StrictMock<MockShutdownTimer>>();

    EXPECT_CALL(*susiWrapperFactory, createSusiWrapper(_)).WillOnce(Return(susiWrapper));

    SusiResult susiResult = SUSI_S_OK;
    SusiScanResult scanResult;
    scanResult.scanResultJson = const_cast<char*>(cleanSusiResponseStr.c_str());
    std::string filePath = "/tmp/clean_file.txt";

    EXPECT_CALL(*susiWrapper, scanFile(_, filePath.c_str(), _, _)).WillOnce(DoAll(SetArgPointee<3>(&scanResult), Return(susiResult)));
    EXPECT_CALL(*susiWrapper, freeResult(&scanResult));
    EXPECT_CALL(*mock_timer, reset()).Times(1);

    threat_scanner::SusiScanner susiScanner(susiWrapperFactory, false, false, nullptr, mock_timer);
    datatypes::AutoFd fd(100);
    scan_messages::ScanResponse response = susiScanner.scan(fd, filePath, scan_messages::E_SCAN_TYPE_ON_DEMAND, "root");
    static_cast<void>(fd.release()); // not a real file descriptor

    EXPECT_FALSE(appenderContains("Failed to parse SUSI response:"));
    EXPECT_EQ(response.allClean(), true);
}

TEST_F(TestThreatScanner, test_SusiScanner_scanFile_threat)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    setupFakeSophosThreatDetectorConfig();

    WaitForEvent serverWaitGuard;
    auto susiWrapper = std::make_shared<MockSusiWrapper>("");
    auto susiWrapperFactory = std::make_shared<StrictMock<MockSusiWrapperFactory>>();
    auto mock_reporter = std::make_shared<StrictMock<MockIThreatReporter>>();
    auto mock_timer = std::make_shared<StrictMock<MockShutdownTimer>>();

    EXPECT_CALL(*susiWrapperFactory, createSusiWrapper(_)).WillOnce(Return(susiWrapper));

    EXPECT_CALL(*mock_reporter, sendThreatReport(_, _, _, _, _, _)).Times(1).WillOnce(
        InvokeWithoutArgs(&serverWaitGuard, &WaitForEvent::onEventNoArgs));
    EXPECT_CALL(*mock_timer, reset()).Times(1);

    SusiResult susiResult = SUSI_I_THREATPRESENT;
    SusiScanResult scanResult;
    scanResult.version = 1;
    scanResult.scanResultJson = const_cast<char*>(singleDetectionSusiResponseStr.c_str());
    std::string filePath = "/tmp/eicar.txt";

    EXPECT_CALL(*susiWrapper, scanFile(_, filePath.c_str(), _, _)).WillOnce(DoAll(SetArgPointee<3>(&scanResult), Return(susiResult)));
    EXPECT_CALL(*susiWrapper, freeResult(&scanResult));

    threat_scanner::SusiScanner susiScanner(susiWrapperFactory, false, false, mock_reporter, mock_timer);
    datatypes::AutoFd fd(101);
    scan_messages::ScanResponse response = susiScanner.scan(fd, filePath, scan_messages::E_SCAN_TYPE_ON_DEMAND, "root");
    static_cast<void>(fd.release()); // Not a real file descriptor

    serverWaitGuard.wait();

    EXPECT_FALSE(appenderContains("Failed to parse SUSI response:"));
    EXPECT_TRUE(appenderContains("Detected \"MAL/malware-A\" in /tmp/eicar.txt (On Demand)"));
    EXPECT_EQ(response.allClean(), false);
}

TEST_F(TestThreatScanner, Test_SusiScanner_MultipleThreats)
{

}

TEST_F(TestThreatScanner, Test_SusiScanner_Scan_Error_And_Clean_Scan_And_No_SUSI_Error)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    setupFakeSophosThreatDetectorConfig();

    WaitForEvent serverWaitGuard;
    auto susiWrapper = std::make_shared<MockSusiWrapper>("");
    auto susiWrapperFactory = std::make_shared<StrictMock<MockSusiWrapperFactory>>();
    auto mock_reporter = std::make_shared<StrictMock<MockIThreatReporter>>();
    auto mock_timer = std::make_shared<StrictMock<MockShutdownTimer>>();

    EXPECT_CALL(*mock_timer, reset()).Times(1);
    EXPECT_CALL(*susiWrapperFactory, createSusiWrapper(_)).WillOnce(Return(susiWrapper));

    SusiResult susiResult = SUSI_S_OK;
    SusiScanResult scanResult;
    scanResult.version = 1;
    scanResult.scanResultJson = const_cast<char*>(errorSusiResponseStr.c_str());
    std::string filePath = "/tmp/eicar.txt";

    EXPECT_CALL(*susiWrapper, scanFile(_, filePath.c_str(), _, _)).WillOnce(DoAll(SetArgPointee<3>(&scanResult), Return(susiResult)));
    EXPECT_CALL(*susiWrapper, freeResult(&scanResult));

    threat_scanner::SusiScanner susiScanner(susiWrapperFactory, false, false, mock_reporter, mock_timer);
    datatypes::AutoFd fd(101);
    scan_messages::ScanResponse response = susiScanner.scan(fd, filePath, scan_messages::E_SCAN_TYPE_ON_DEMAND, "root");
    static_cast<void>(fd.release()); // Not a real file descriptor

    EXPECT_FALSE(appenderContains("Failed to parse SUSI response:"));
    EXPECT_FALSE(appenderContains("Detected "));
    EXPECT_EQ(response.allClean(), true);
    EXPECT_EQ(response.getErrorMsg(), "Failed to scan /tmp/eicar.txt as it is password protected");
}

TEST_F(TestThreatScanner, Test_SusiScanner_Scan_Error_And_Clean_Scan_And_SUSI_Error)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    setupFakeSophosThreatDetectorConfig();

    WaitForEvent serverWaitGuard;
    auto susiWrapper = std::make_shared<MockSusiWrapper>("");
    auto susiWrapperFactory = std::make_shared<StrictMock<MockSusiWrapperFactory>>();
    auto mock_reporter = std::make_shared<StrictMock<MockIThreatReporter>>();
    auto mock_timer = std::make_shared<StrictMock<MockShutdownTimer>>();

    EXPECT_CALL(*mock_timer, reset()).Times(1);
    EXPECT_CALL(*susiWrapperFactory, createSusiWrapper(_)).WillOnce(Return(susiWrapper));

    SusiResult susiResult = SUSI_E_OUTOFMEMORY;
    SusiScanResult scanResult;
    scanResult.version = 1;
    scanResult.scanResultJson = const_cast<char*>(errorSusiResponseStr.c_str());
    std::string filePath = "/tmp/eicar.txt";

    EXPECT_CALL(*susiWrapper, scanFile(_, filePath.c_str(), _, _)).WillOnce(DoAll(SetArgPointee<3>(&scanResult), Return(susiResult)));
    EXPECT_CALL(*susiWrapper, freeResult(&scanResult));

    threat_scanner::SusiScanner susiScanner(susiWrapperFactory, false, false, mock_reporter, mock_timer);
    datatypes::AutoFd fd(101);
    scan_messages::ScanResponse response = susiScanner.scan(fd, filePath, scan_messages::E_SCAN_TYPE_ON_DEMAND, "root");
    static_cast<void>(fd.release()); // Not a real file descriptor

    EXPECT_FALSE(appenderContains("Failed to parse SUSI response:"));
    EXPECT_FALSE(appenderContains("Detected "));
    EXPECT_EQ(response.allClean(), true);
    EXPECT_EQ(response.getErrorMsg(), "Failed to scan /tmp/eicar.txt as it is password protected");
    EXPECT_TRUE(appenderContains("Failed to scan /tmp/eicar.txt due to a susi out of memory error"));
}

TEST_F(TestThreatScanner, Test_SusiScanner_Scan_Error_And_Threat_Present_And_No_Susi_Error)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    setupFakeSophosThreatDetectorConfig();

    WaitForEvent serverWaitGuard;
    auto susiWrapper = std::make_shared<MockSusiWrapper>("");
    auto susiWrapperFactory = std::make_shared<StrictMock<MockSusiWrapperFactory>>();
    auto mock_reporter = std::make_shared<StrictMock<MockIThreatReporter>>();
    auto mock_timer = std::make_shared<StrictMock<MockShutdownTimer>>();

    EXPECT_CALL(*susiWrapperFactory, createSusiWrapper(_)).WillOnce(Return(susiWrapper));

    EXPECT_CALL(*mock_reporter, sendThreatReport(_, _, _, _, _, _)).Times(1).WillOnce(
        InvokeWithoutArgs(&serverWaitGuard, &WaitForEvent::onEventNoArgs));
    EXPECT_CALL(*mock_timer, reset()).Times(1);

    SusiResult susiResult = SUSI_I_THREATPRESENT;
    SusiScanResult scanResult;
    scanResult.version = 1;
    scanResult.scanResultJson = const_cast<char*>(errorSusiResponseStr.c_str());
    std::string filePath = "/tmp/eicar.txt";

    EXPECT_CALL(*susiWrapper, scanFile(_, filePath.c_str(), _, _)).WillOnce(DoAll(SetArgPointee<3>(&scanResult), Return(susiResult)));
    EXPECT_CALL(*susiWrapper, freeResult(&scanResult));

    threat_scanner::SusiScanner susiScanner(susiWrapperFactory, false, false, mock_reporter, mock_timer);
    datatypes::AutoFd fd(101);
    scan_messages::ScanResponse response = susiScanner.scan(fd, filePath, scan_messages::E_SCAN_TYPE_ON_DEMAND, "root");
    static_cast<void>(fd.release()); // Not a real file descriptor

    serverWaitGuard.wait();

    EXPECT_FALSE(appenderContains("Failed to parse SUSI response:"));
    EXPECT_FALSE(appenderContains("Detected "));
    EXPECT_EQ(response.allClean(), false);
    EXPECT_EQ(response.getDetections().front().path, "/tmp/eicar.txt");
    EXPECT_EQ(response.getDetections().front().name, "unknown");
    EXPECT_EQ(response.getDetections().front().sha256, "unknown");
    EXPECT_EQ(response.getErrorMsg(), "Failed to scan /tmp/eicar.txt as it is password protected");
}

TEST_F(TestThreatScanner, Test_SusiScanner_BadXML)
{

}

TEST_F(TestThreatScanner, TestsusiResultErrorToReadableErrorUnknown)
{
    auto loglevel = log4cplus::DEBUG_LOG_LEVEL;
    EXPECT_EQ(threat_scanner::SusiScanner::susiResultErrorToReadableError("test.file", static_cast<SusiResult>(17), loglevel), "Failed to scan test.file due to an unknown susi error [17]");

    EXPECT_EQ(loglevel, log4cplus::ERROR_LOG_LEVEL);
}

TEST(TestThrowIfNotOk, TestOk)
{
    EXPECT_NO_THROW(throwIfNotOk(SUSI_S_OK, "Should not throw"));
}

TEST(TestThrowIfNotOk, TestNotOk)
{
    EXPECT_THROW(throwIfNotOk(SUSI_E_BAD_JSON, "Should throw"), std::runtime_error);
}


class ThreatScannerParameterizedTest
    : public ::testing::TestWithParam<std::tuple<std::string, std::string, log4cplus::LogLevel > >
{
};

INSTANTIATE_TEST_SUITE_P(TestThreatScanner, ThreatScannerParameterizedTest, ::testing::Values(
    std::make_tuple("encrypted", "Failed to scan test.file as it is password protected", log4cplus::WARN_LOG_LEVEL),
    std::make_tuple("corrupt", "Failed to scan test.file as it is corrupted", log4cplus::ERROR_LOG_LEVEL),
    std::make_tuple("unsupported", "Failed to scan test.file as it is not a supported file type", log4cplus::WARN_LOG_LEVEL),
    std::make_tuple("couldn't open", "Failed to scan test.file as it could not be opened", log4cplus::ERROR_LOG_LEVEL),
    std::make_tuple("recursion limit", "Failed to scan test.file as it is a Zip Bomb", log4cplus::ERROR_LOG_LEVEL),
    std::make_tuple("scan failed", "Failed to scan test.file due to a sweep failure", log4cplus::ERROR_LOG_LEVEL),
    std::make_tuple("unexpected (0x80040231)", "Failed to scan test.file [unexpected (0x80040231)]", log4cplus::ERROR_LOG_LEVEL)
));

TEST_P(ThreatScannerParameterizedTest, susiErrorToReadableError)
{
    auto loglevel = log4cplus::DEBUG_LOG_LEVEL;
    EXPECT_EQ(threat_scanner::SusiScanner::susiErrorToReadableError("test.file", std::get<0>(GetParam()), loglevel), std::get<1>(GetParam()));
    EXPECT_EQ(loglevel, std::get<2>(GetParam()));
}

class SusiResultErrorToReadableErrorParameterized
    : public ::testing::TestWithParam<std::tuple<SusiResult, std::string, log4cplus::LogLevel>>
{
};

INSTANTIATE_TEST_SUITE_P(TestThreatScanner, SusiResultErrorToReadableErrorParameterized, ::testing::Values(
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
));

TEST_P(SusiResultErrorToReadableErrorParameterized, susiResultErrorToReadableError)
{
    auto loglevel = log4cplus::DEBUG_LOG_LEVEL;
    EXPECT_EQ(threat_scanner::SusiScanner::susiResultErrorToReadableError("test.file", std::get<0>(GetParam()), loglevel), std::get<1>(GetParam()));
    EXPECT_EQ(loglevel, std::get<2>(GetParam()));
}