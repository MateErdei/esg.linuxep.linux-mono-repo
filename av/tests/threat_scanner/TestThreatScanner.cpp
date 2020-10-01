/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "MockSusiWrapper.h"
#include "MockSusiWrapperFactory.h"

#include "sophos_threat_detector/threat_scanner/SusiScanner.h"
#include "sophos_threat_detector/threat_scanner/ThrowIfNotOk.h"

#include "unixsocket/threatReporterSocket/ThreatReporterServerSocket.h"

#include "../common/Common.h"
#include "../common/LogInitializedTests.h"
#include "../common/WaitForEvent.h"

#include "Common/UtilityImpl/StringUtils.h"
#include <Common/Logging/ConsoleLoggingSetup.h>

#include <csignal>

using namespace testing;

static SetupTestLogging consoleLoggingSetup;

namespace
{
    class IgnoreSigPipe
    {
    public:
        IgnoreSigPipe() noexcept
        {
            signal(SIGPIPE, SIG_IGN);
        }
        ~IgnoreSigPipe() noexcept
        {
            signal(SIGPIPE, SIG_DFL);
        }
    };
}

static IgnoreSigPipe sig_pipe_ignorer;

static const std::string susiResponseStr =
        "{\n"
        "    \"results\":\n"
        "    [\n"
        "        {\n"
        "            \"detections\": [\n"
        "               {\n"
        "                   \"threatName\": \"MAL/malware-A\",\n"
        "                   \"threatType\": \"malware/trojan/PUA/...\",\n"
        "                   \"threatPath\": \"...\",\n"
        "                   \"longName\": \"...\",\n"
        "                   \"identityType\": 0,\n"
        "                   \"identitySubtype\": 0\n"
        "               }\n"
        "            ],\n"
        "            \"Local\": {\n"
        "                \"LookupType\": 1,\n"
        "                \"Score\": 20,\n"
        "                \"SignerStrength\": 30\n"
        "            },\n"
        "            \"Global\": {\n"
        "                \"LookupType\": 1,\n"
        "                \"Score\": 40\n"
        "            },\n"
        "            \"ML\": {\n"
        "                \"ml-pe\": 50\n"
        "            },\n"
        "            \"properties\": {\n"
        "                \"GENES/SUPPRESSML\": 1\n"
        "            },\n"
        "            \"telemetry\": [\n"
        "                {\n"
        "                    \"identityName\": \"xxx\",\n"
        "                    \"dataHex\": \"6865646765686f67\"\n"
        "                }\n"
        "            ],\n"
        "            \"mlscores\": [\n"
        "                {\n"
        "                    \"score\": 12,\n"
        "                    \"secScore\": 34,\n"
        "                    \"featuresHex\": \"6865646765686f67\",\n"
        "                    \"mlDataVersion\": 56\n"
        "                }\n"
        "            ]\n"
        "        }\n"
        "    ]\n"
        "}";

class MockIThreatReportCallbacks : public IMessageCallback
{
public:
    MOCK_METHOD1(processMessage, void(const std::string& threatDetectedXML));
};

TEST(TestThreatScanner, test_SusiScannerConstruction) //NOLINT
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
                "macintosh": true
            },
            "scanControl": {
                "trueFileTypeDetection": true,
                "puaDetection": false,
                "archiveRecursionDepth": 16,
                "stopOnArchiveBombs": true
            }
        }
    })";

    std::string scannerConfig = Common::UtilityImpl::StringUtils::orderedStringReplace(
        "{@@SCANNER_CONFIG@@}", {{"@@SCANNER_CONFIG@@", scannerInfo} });

    auto susiWrapper = std::make_shared<MockSusiWrapper>(scannerConfig);
    std::shared_ptr<MockSusiWrapperFactory> susiWrapperFactory = std::make_shared<MockSusiWrapperFactory>();

    EXPECT_CALL(*susiWrapperFactory, createSusiWrapper(scannerConfig)).WillOnce(Return(susiWrapper));

    threat_scanner::SusiScanner susiScanner(susiWrapperFactory, false);
}

TEST(TestThreatScanner, test_SusiScannerConstructionWithScanArchives) //NOLINT
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
                "macintosh": true
            },
            "scanControl": {
                "trueFileTypeDetection": true,
                "puaDetection": false,
                "archiveRecursionDepth": 16,
                "stopOnArchiveBombs": true
            }
        }
    })";

    auto scannerConfig = Common::UtilityImpl::StringUtils::orderedStringReplace(
        "{@@SCANNER_CONFIG@@}", {{"@@SCANNER_CONFIG@@", scannerInfo} });


    auto susiWrapper = std::make_shared<MockSusiWrapper>(scannerConfig);
    std::shared_ptr<MockSusiWrapperFactory> susiWrapperFactory = std::make_shared<MockSusiWrapperFactory>();

    EXPECT_CALL(*susiWrapperFactory, createSusiWrapper(scannerConfig)).WillOnce(Return(susiWrapper));

    threat_scanner::SusiScanner susiScanner(susiWrapperFactory, true);
}

TEST(TestThreatScanner, test_SusiScanner_scanFile_clean) //NOLINT
{
    setupFakeSophosThreatDetectorConfig();

    auto susiWrapper = std::make_shared<MockSusiWrapper>("");
    auto susiWrapperFactory = std::make_shared<MockSusiWrapperFactory>();

    EXPECT_CALL(*susiWrapperFactory, createSusiWrapper(_)).WillOnce(Return(susiWrapper));

    SusiResult susiResult = SUSI_S_OK;
    SusiScanResult* scanResult = nullptr;
    std::string filePath = "/tmp/clean_file.txt";

    EXPECT_CALL(*susiWrapper, scanFile(_, filePath.c_str(), _, _)).WillOnce(Return(susiResult));
    EXPECT_CALL(*susiWrapper, freeResult(scanResult));

    threat_scanner::SusiScanner susiScanner(susiWrapperFactory, false);
    datatypes::AutoFd fd(100);
    scan_messages::ScanResponse response = susiScanner.scan(fd, filePath, scan_messages::E_SCAN_TYPE_ON_DEMAND, "root");
    static_cast<void>(fd.release()); // not a real file descriptor

    EXPECT_EQ(response.allClean(), true);
}

TEST(TestThreatScanner, test_SusiScanner_scanFile_threat) //NOLINT
{
    setupFakeSophosThreatDetectorConfig();

    WaitForEvent serverWaitGuard;
    auto susiWrapper = std::make_shared<MockSusiWrapper>("");
    std::shared_ptr<MockSusiWrapperFactory> susiWrapperFactory = std::make_shared<MockSusiWrapperFactory>();
    std::shared_ptr<StrictMock<MockIThreatReportCallbacks> > mock_callback = std::make_shared<StrictMock<MockIThreatReportCallbacks>>();

    EXPECT_CALL(*susiWrapperFactory, createSusiWrapper(_)).WillOnce(Return(susiWrapper));

    EXPECT_CALL(*mock_callback, processMessage(_)).Times(1).WillOnce(
        InvokeWithoutArgs(&serverWaitGuard, &WaitForEvent::onEventNoArgs));

    setupFakeSophosThreatDetectorConfig();
    unixsocket::ThreatReporterServerSocket threatReporterServer(
        "/tmp/TestPluginAdapter/chroot/threat_report_socket", 0600,
        mock_callback
    );

    threatReporterServer.start();

    SusiResult susiResult = SUSI_I_THREATPRESENT;
    SusiScanResult scanResult;
    scanResult.version = 1;
    scanResult.scanResultJson = const_cast<char*>(susiResponseStr.c_str());
    std::string filePath = "/tmp/eicar.txt";

    EXPECT_CALL(*susiWrapper, scanFile(_, filePath.c_str(), _, _)).WillOnce(DoAll(SetArgPointee<3>(&scanResult), Return(susiResult)));
    EXPECT_CALL(*susiWrapper, freeResult(&scanResult));

    threat_scanner::SusiScanner susiScanner(susiWrapperFactory, false);
    datatypes::AutoFd fd(101);
    scan_messages::ScanResponse response = susiScanner.scan(fd, filePath, scan_messages::E_SCAN_TYPE_ON_DEMAND, "root");
    static_cast<void>(fd.release()); // Not a real file descriptor

    serverWaitGuard.wait();
    threatReporterServer.requestStop();
    threatReporterServer.join();

    EXPECT_EQ(response.allClean(), false);
}

TEST(TestThrowIfNotOk, TestOk) // NOLINT
{
    EXPECT_NO_THROW(throwIfNotOk(SUSI_S_OK, "Should not throw"));
}

TEST(TestThrowIfNotOk, TestNotOk) // NOLINT
{
    EXPECT_THROW(throwIfNotOk(SUSI_E_BAD_JSON, "Should throw"), std::runtime_error);
}
