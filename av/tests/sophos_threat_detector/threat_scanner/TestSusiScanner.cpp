// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "capnp/message.h"
#include "scan_messages/ScanRequest.h"
#include "sophos_threat_detector/threat_scanner/IUnitScanner.h"
#include "sophos_threat_detector/threat_scanner/SusiResultUtils.h"
#include "sophos_threat_detector/threat_scanner/SusiScanner.h"
#include "sophos_threat_detector/threat_scanner/ThreatDetectedBuilder.h"

// test includes
#include "tests/common/Common.h"
#include "tests/common/MemoryAppender.h"
#include "tests/common/WaitForEvent.h"
#include "tests/sophos_threat_detector/sophosthreatdetectorimpl/MockShutdownTimer.h"
#include "tests/sophos_threat_detector/sophosthreatdetectorimpl/MockThreatReporter.h"

#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFileSystem.h"

#include <gmock/gmock.h>

using namespace threat_scanner;
using namespace testing;
using scan_messages::MetadataRescan, scan_messages::MetadataRescanResponse;
using MetadataResponse = scan_messages::MetadataRescanResponse;

namespace
{
    class MockUnitScanner : public IUnitScanner
    {
    public:
        MOCK_METHOD(ScanResult, scan, (datatypes::AutoFd & fd, const std::string& path), (override));
        MOCK_METHOD(
            scan_messages::MetadataRescanResponse,
            metadataRescan,
            (std::string_view threatType, std::string_view threatName, std::string_view path, std::string_view sha256),
            (override));
    };

    class MockSusiGlobalHandler : public ISusiGlobalHandler
    {
    public:
        MOCK_METHOD(bool, isAllowListedSha256, (const std::string& threatCheckSum), (override));
        MOCK_METHOD(bool, isAllowListedPath, (const std::string& threatPath), (override));
        MOCK_METHOD(bool, isPuaApproved, (const std::string& puaName), (override));
        MOCK_METHOD(void, loadSusiSettingsIfRequired, (), (override));
    };

    class TestSusiScanner : public MemoryAppenderUsingTests
    {
    public:
        TestSusiScanner() : MemoryAppenderUsingTests("ThreatScanner")
        {
        }
        void SetUp() override
        {
            m_mockThreatReporter = std::make_shared<NiceMock<MockThreatReporter>>();
            m_mockTimer = std::make_shared<StrictMock<MockShutdownTimer>>();
            m_mockSusiGlobalHandler = std::make_shared<NaggyMock<MockSusiGlobalHandler>>();

            ON_CALL(*m_mockSusiGlobalHandler, isAllowListedPath(_)).WillByDefault(Return(false));
            ON_CALL(*m_mockSusiGlobalHandler, isAllowListedSha256(_)).WillByDefault(Return(false));
        }

        std::shared_ptr<MockThreatReporter> m_mockThreatReporter;
        std::shared_ptr<MockShutdownTimer> m_mockTimer;
        std::shared_ptr<MockSusiGlobalHandler> m_mockSusiGlobalHandler;

        static scan_messages::ScanRequest makeScanRequestObject(
            const std::string& path,
            const scan_messages::E_SCAN_TYPE scanType = scan_messages::E_SCAN_TYPE_ON_DEMAND,
            const std::string& userId = "root",
            int64_t pid = -1,
            const std::string& exepath = "")
        {
            ::capnp::MallocMessageBuilder message;
            Sophos::ssplav::FileScanRequest::Builder requestBuilder =
                message.initRoot<Sophos::ssplav::FileScanRequest>();
            requestBuilder.setPathname(path);
            requestBuilder.setScanType(scanType);
            requestBuilder.setUserID(userId);
            requestBuilder.setPid(pid);
            requestBuilder.setExecutablePath(exepath);

            Sophos::ssplav::FileScanRequest::Reader requestReader = requestBuilder;

            return scan_messages::ScanRequest(requestReader);
        }

        static void expectCorrectThreatDetected(
            const scan_messages::ThreatDetected& threatDetected,
            const std::string& path,
            const std::string& threatName,
            const std::string& threatType,
            const std::string& sha256,
            const std::string& threatSha256,
            const scan_messages::E_SCAN_TYPE scanType = scan_messages::E_SCAN_TYPE_ON_DEMAND)
        {
            EXPECT_EQ(threatDetected.userID, "root");
            EXPECT_EQ(threatDetected.threatType, threatType);
            EXPECT_EQ(threatDetected.threatName, threatName);
            EXPECT_EQ(threatDetected.scanType, scanType);
            EXPECT_EQ(threatDetected.filePath, path);
            EXPECT_EQ(threatDetected.sha256, sha256);
            EXPECT_EQ(threatDetected.threatSha256, threatSha256);
            EXPECT_EQ(threatDetected.threatId, generateThreatId(path, sha256));
            EXPECT_EQ(threatDetected.isRemote, false);
            EXPECT_EQ(threatDetected.reportSource, threat_scanner::getReportSource(threatName));
        }

        SusiScanner makeScanner(IUnitScanner* scanner) const
        {
            return makeScanner(std::unique_ptr<IUnitScanner>{ scanner }, nullptr, m_mockTimer, m_mockSusiGlobalHandler);
        }

        SusiScanner makeScannerWithReporter(IUnitScanner* scanner) const
        {
            return makeScanner(
                std::unique_ptr<IUnitScanner>{ scanner }, m_mockThreatReporter, m_mockTimer, m_mockSusiGlobalHandler);
        }

        static SusiScanner makeScanner(std::unique_ptr<IUnitScanner> scanner, IScanNotificationSharedPtr timer)
        {
            return makeScanner(std::move(scanner), nullptr, std::move(timer));
        }

        static SusiScanner makeScanner(
            std::unique_ptr<IUnitScanner> scanner,
            IThreatReporterSharedPtr threatReporter,
            IScanNotificationSharedPtr timer)
        {
            return SusiScanner{ std::move(scanner), std::move(threatReporter), std::move(timer), nullptr };
        }

        static SusiScanner makeScanner(
            std::unique_ptr<IUnitScanner> scanner,
            IThreatReporterSharedPtr threatReporter,
            IScanNotificationSharedPtr timer,
            ISusiGlobalHandlerSharedPtr allowList)
        {
            return SusiScanner{ std::move(scanner), std::move(threatReporter), std::move(timer), std::move(allowList) };
        }
    };
} // namespace

TEST_F(TestSusiScanner, scan_CleanScan)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto* mockUnitScanner = new StrictMock<MockUnitScanner>();

    SusiScanner susiScanner{ makeScanner(mockUnitScanner) }; // DONATED scanner

    EXPECT_CALL(*m_mockTimer, reset()).Times(1);

    ScanResult scanResult{ {}, {} };
    EXPECT_CALL(*mockUnitScanner, scan(_, "/tmp/clean_file.txt")).Times(1).WillOnce(Return(scanResult));

    datatypes::AutoFd autoFd;

    scan_messages::ScanResponse response = susiScanner.scan(autoFd, makeScanRequestObject("/tmp/clean_file.txt"));

    EXPECT_FALSE(appenderContains("Detected "));
    EXPECT_FALSE(appenderContains("Sending report for detection "));
    EXPECT_TRUE(response.allClean());
    EXPECT_EQ(response.getErrorMsg(), "");
}

TEST_F(TestSusiScanner, scan_DetectionWithoutErrors_SendsReport)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const auto mockUnitScanner = new StrictMock<MockUnitScanner>();

    SusiScanner susiScanner{ makeScannerWithReporter(mockUnitScanner) };

    EXPECT_CALL(*m_mockTimer, reset()).Times(1);
    EXPECT_CALL(*m_mockSusiGlobalHandler, isPuaApproved(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockSusiGlobalHandler, isAllowListedPath(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockSusiGlobalHandler, isAllowListedSha256(_)).WillRepeatedly(Return(false));

    ScanResult scanResult{ { { "/tmp/eicar.txt", "threatName", "threatType", "sha256" } }, {} };
    EXPECT_CALL(*mockUnitScanner, scan(_, "/tmp/eicar.txt")).Times(1).WillOnce(Return(scanResult));

    WaitForEvent serverWaitGuard;
    EXPECT_CALL(*m_mockThreatReporter, sendThreatReport(_))
        .Times(1)
        .WillOnce(Invoke(
            [&](const scan_messages::ThreatDetected& threatDetected)
            {
                expectCorrectThreatDetected(
                    threatDetected, "/tmp/eicar.txt", "threatName", "threatType", "sha256", "sha256");
                EXPECT_EQ(threatDetected.pid, -1);
                serverWaitGuard.onEventNoArgs();
            }));

    datatypes::AutoFd autoFd;

    scan_messages::ScanResponse response = susiScanner.scan(autoFd, makeScanRequestObject("/tmp/eicar.txt"));

    serverWaitGuard.wait();

    EXPECT_FALSE(response.allClean());
    ASSERT_EQ(response.getDetections().size(), 1);
    EXPECT_EQ(response.getDetections().at(0).path, "/tmp/eicar.txt");
    EXPECT_EQ(response.getDetections().at(0).name, "threatName");
    EXPECT_EQ(response.getDetections().at(0).sha256, "sha256");
    EXPECT_EQ(response.getErrorMsg(), "");

    EXPECT_TRUE(appenderContains("Detected \"threatName\" in /tmp/eicar.txt (On Demand)"));
    EXPECT_TRUE(appenderContains("Sending report for detection 'threatName' in /tmp/eicar.txt"));
}

TEST_F(TestSusiScanner, scan_DetectionIsAllowedByShaAndReturnsEarly)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const auto mockUnitScanner = new StrictMock<MockUnitScanner>();

    SusiScanner susiScanner{ makeScannerWithReporter(mockUnitScanner) };

    EXPECT_CALL(*m_mockTimer, reset()).Times(1);
    EXPECT_CALL(*m_mockSusiGlobalHandler, isPuaApproved(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockSusiGlobalHandler, isAllowListedPath(_)).WillRepeatedly(Return(false));

    // These expects are important for the test
    EXPECT_CALL(*m_mockSusiGlobalHandler, isAllowListedSha256(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockThreatReporter, sendThreatReport(_)).Times(0);

    ScanResult scanResult{ { { "/tmp/eicar.txt", "threatName", "threatType", "sha256" } }, {} };
    EXPECT_CALL(*mockUnitScanner, scan(_, "/tmp/eicar.txt")).Times(1).WillOnce(Return(scanResult));

    datatypes::AutoFd autoFd;
    scan_messages::ScanResponse response = susiScanner.scan(autoFd, makeScanRequestObject("/tmp/eicar.txt"));

    EXPECT_TRUE(response.allClean());
    EXPECT_EQ(response.getDetections().size(), 0);

    EXPECT_TRUE(appenderContains("Allowing /tmp/eicar.txt with sha256"));
}

TEST_F(TestSusiScanner, scan_DetectionOnaccessSetPid)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const auto mockUnitScanner = new StrictMock<MockUnitScanner>();

    SusiScanner susiScanner{ makeScannerWithReporter(mockUnitScanner) };

    EXPECT_CALL(*m_mockTimer, reset()).Times(1);

    ScanResult scanResult{ { { "/tmp/eicar.txt", "threatName", "threatType", "sha256" } }, {} };
    EXPECT_CALL(*mockUnitScanner, scan(_, "/tmp/eicar.txt")).Times(1).WillOnce(Return(scanResult));

    WaitForEvent serverWaitGuard;
    EXPECT_CALL(*m_mockThreatReporter, sendThreatReport(_))
        .Times(1)
        .WillOnce(Invoke(
            [&](const scan_messages::ThreatDetected& threatDetected)
            {
                expectCorrectThreatDetected(
                    threatDetected,
                    "/tmp/eicar.txt",
                    "threatName",
                    "threatType",
                    "sha256",
                    "sha256",
                    scan_messages::E_SCAN_TYPE_ON_ACCESS);
                EXPECT_EQ(threatDetected.pid, 100);
                EXPECT_EQ(threatDetected.executablePath, "/random/path/of/parent");
                serverWaitGuard.onEventNoArgs();
            }));

    datatypes::AutoFd autoFd;
    scan_messages::ScanRequest request = makeScanRequestObject(
        "/tmp/eicar.txt", scan_messages::E_SCAN_TYPE_ON_ACCESS, "root", 100, "/random/path/of/parent");

    scan_messages::ScanResponse response = susiScanner.scan(autoFd, request);

    serverWaitGuard.wait();
}
TEST_F(TestSusiScanner, scan_DetectionWithErrors_SendsReport)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto* mockUnitScanner = new StrictMock<MockUnitScanner>();

    SusiScanner susiScanner{ makeScannerWithReporter(mockUnitScanner) };

    EXPECT_CALL(*m_mockTimer, reset()).Times(1);
    EXPECT_CALL(*m_mockSusiGlobalHandler, isPuaApproved(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockSusiGlobalHandler, isAllowListedPath(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockSusiGlobalHandler, isAllowListedSha256(_)).WillRepeatedly(Return(false));

    ScanResult scanResult{ { { "/tmp/eicar.txt", "threatName", "threatType", "sha256" } },
                           { { "info message", log4cplus::INFO_LOG_LEVEL },
                             { "warn message", log4cplus::WARN_LOG_LEVEL },
                             { "error message", log4cplus::ERROR_LOG_LEVEL } } };
    EXPECT_CALL(*mockUnitScanner, scan(_, "/tmp/eicar.txt")).Times(1).WillOnce(Return(scanResult));

    WaitForEvent serverWaitGuard;
    EXPECT_CALL(*m_mockThreatReporter, sendThreatReport(_))
        .Times(1)
        .WillOnce(Invoke(
            [&](const scan_messages::ThreatDetected& threatDetected)
            {
                expectCorrectThreatDetected(
                    threatDetected, "/tmp/eicar.txt", "threatName", "threatType", "sha256", "sha256");
                serverWaitGuard.onEventNoArgs();
            }));

    datatypes::AutoFd autoFd;

    scan_messages::ScanResponse response = susiScanner.scan(autoFd, makeScanRequestObject("/tmp/eicar.txt"));

    serverWaitGuard.wait();

    EXPECT_FALSE(response.allClean());
    ASSERT_EQ(response.getDetections().size(), 1);
    EXPECT_EQ(response.getDetections().at(0).path, "/tmp/eicar.txt");
    EXPECT_EQ(response.getDetections().at(0).name, "threatName");
    EXPECT_EQ(response.getDetections().at(0).sha256, "sha256");
    EXPECT_EQ(response.getErrorMsg(), "info message");

    EXPECT_TRUE(appenderContains("Detected \"threatName\" in /tmp/eicar.txt (On Demand)"));
    EXPECT_TRUE(appenderContains("Sending report for detection 'threatName' in /tmp/eicar.txt"));

    EXPECT_TRUE(appenderContains("info message"));
    EXPECT_TRUE(appenderContains("warn message"));
    EXPECT_TRUE(appenderContains("error message"));
}

TEST_F(TestSusiScanner, scan_NoDetectionWithErrors_DoesntSendReport)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const auto mockUnitScanner = new StrictMock<MockUnitScanner>();

    SusiScanner susiScanner{ makeScanner(mockUnitScanner) };

    EXPECT_CALL(*m_mockTimer, reset()).Times(1);

    ScanResult scanResult{ {}, { { "error message", log4cplus::ERROR_LOG_LEVEL } } };
    EXPECT_CALL(*mockUnitScanner, scan(_, "/tmp/clean_file.txt")).Times(1).WillOnce(Return(scanResult));

    datatypes::AutoFd autoFd;

    scan_messages::ScanResponse response = susiScanner.scan(autoFd, makeScanRequestObject("/tmp/clean_file.txt"));

    EXPECT_TRUE(response.allClean());
    EXPECT_EQ(response.getDetections().size(), 0);
    EXPECT_EQ(response.getErrorMsg(), "error message");

    EXPECT_FALSE(appenderContains("Detected "));
    EXPECT_TRUE(appenderContains("error message"));
}

TEST_F(TestSusiScanner, scan_ShorterPathInDetection_ReportSentWithOriginalPath)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto* mockUnitScanner = new StrictMock<MockUnitScanner>();

    EXPECT_CALL(*m_mockTimer, reset()).Times(1);

    ScanResult scanResult{ { { "/tmp", "threatName", "threatType", "sha256" } }, {} };
    EXPECT_CALL(*mockUnitScanner, scan(_, "/tmp/eicar.txt")).Times(1).WillOnce(Return(scanResult));

    // Needed because of path mismatch
    auto mockFileSystem = new StrictMock<MockFileSystem>{};
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem = std::unique_ptr<IFileSystem>(mockFileSystem);
    EXPECT_CALL(*mockFileSystem, calculateDigest(_, -1)).Times(1).WillOnce(Return("calculatedSha256"));

    WaitForEvent serverWaitGuard;
    EXPECT_CALL(*m_mockThreatReporter, sendThreatReport(_))
        .Times(1)
        .WillOnce(Invoke(
            [&](const scan_messages::ThreatDetected& threatDetected)
            {
                expectCorrectThreatDetected(
                    threatDetected, "/tmp/eicar.txt", "threatName", "threatType", "calculatedSha256", "sha256");
                serverWaitGuard.onEventNoArgs();
            }));

    SusiScanner susiScanner{ makeScannerWithReporter(mockUnitScanner) }; // DONATED

    datatypes::AutoFd autoFd;

    scan_messages::ScanResponse response = susiScanner.scan(autoFd, makeScanRequestObject("/tmp/eicar.txt"));

    serverWaitGuard.wait();

    EXPECT_FALSE(response.allClean());
    ASSERT_EQ(response.getDetections().size(), 1);
    EXPECT_EQ(response.getDetections().at(0).path, "/tmp");
    EXPECT_EQ(response.getDetections().at(0).name, "threatName");
    EXPECT_EQ(response.getDetections().at(0).sha256, "sha256");
    EXPECT_EQ(response.getErrorMsg(), "");

    EXPECT_TRUE(appenderContains("Detected \"threatName\" in /tmp (On Demand)"));
    EXPECT_TRUE(appenderContains("Sending report for detection 'threatName' in /tmp/eicar.txt"));
}

TEST_F(TestSusiScanner, scan_ArchiveWithDetectionsIncludingItself_SendsReportForArchiveDetection)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto* mockUnitScanner = new StrictMock<MockUnitScanner>();
    EXPECT_CALL(*m_mockTimer, reset()).Times(1);
    EXPECT_CALL(*m_mockSusiGlobalHandler, isPuaApproved(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockSusiGlobalHandler, isAllowListedPath(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockSusiGlobalHandler, isAllowListedSha256(_)).WillRepeatedly(Return(false));

    WaitForEvent serverWaitGuard;
    EXPECT_CALL(*m_mockThreatReporter, sendThreatReport(_))
        .Times(1)
        .WillOnce(Invoke(
            [&](const scan_messages::ThreatDetected& threatDetected)
            {
                expectCorrectThreatDetected(
                    threatDetected,
                    "/tmp/archive.zip",
                    "threatName_archive",
                    "threatType_archive",
                    "sha256_archive",
                    "sha256_archive");
                serverWaitGuard.onEventNoArgs();
            }));

    ScanResult scanResult{ { { "/tmp/archive.zip", "threatName_archive", "threatType_archive", "sha256_archive" },
                             { "/tmp/archive.zip/eicar1.txt", "threatName_1", "threatType_1", "sha256_1" },
                             { "/tmp/archive.zip/eicar2.txt", "threatName_2", "threatType_2", "sha256_2" } },
                           {} };
    EXPECT_CALL(*mockUnitScanner, scan(_, "/tmp/archive.zip")).Times(1).WillOnce(Return(scanResult));

    SusiScanner susiScanner{ makeScannerWithReporter(mockUnitScanner) };

    datatypes::AutoFd autoFd;

    scan_messages::ScanResponse response = susiScanner.scan(autoFd, makeScanRequestObject("/tmp/archive.zip"));

    serverWaitGuard.wait();

    EXPECT_FALSE(response.allClean());
    ASSERT_EQ(response.getDetections().size(), 3);
    EXPECT_EQ(response.getDetections().at(0).path, "/tmp/archive.zip");
    EXPECT_EQ(response.getDetections().at(0).name, "threatName_archive");
    EXPECT_EQ(response.getDetections().at(0).sha256, "sha256_archive");
    EXPECT_EQ(response.getDetections().at(1).path, "/tmp/archive.zip/eicar1.txt");
    EXPECT_EQ(response.getDetections().at(1).name, "threatName_1");
    EXPECT_EQ(response.getDetections().at(1).sha256, "sha256_1");
    EXPECT_EQ(response.getDetections().at(2).path, "/tmp/archive.zip/eicar2.txt");
    EXPECT_EQ(response.getDetections().at(2).name, "threatName_2");
    EXPECT_EQ(response.getDetections().at(2).sha256, "sha256_2");
    EXPECT_EQ(response.getErrorMsg(), "");

    EXPECT_TRUE(appenderContains("Detected \"threatName_archive\" in /tmp/archive.zip (On Demand)"));
    EXPECT_TRUE(appenderContains("Detected \"threatName_1\" in /tmp/archive.zip/eicar1.txt (On Demand)"));
    EXPECT_TRUE(appenderContains("Detected \"threatName_2\" in /tmp/archive.zip/eicar2.txt (On Demand)"));
    EXPECT_TRUE(appenderContains("Sending report for detection 'threatName_archive' in /tmp/archive.zip"));
}

TEST_F(TestSusiScanner, scan_ArchiveWithDetectionsNotIncludingItself_SendsReportForFirstInnerDetectionWithArchivePath)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto* mockUnitScanner = new StrictMock<MockUnitScanner>();

    SusiScanner susiScanner{ makeScannerWithReporter(mockUnitScanner) };

    EXPECT_CALL(*m_mockTimer, reset()).Times(1);

    ScanResult scanResult{ { { "/tmp/archive.zip/eicar1.txt", "threatName_1", "threatType_1", "sha256_1" },
                             { "/tmp/archive.zip/eicar2.txt", "threatName_2", "threatType_2", "sha256_2" } },
                           {} };
    EXPECT_CALL(*mockUnitScanner, scan(_, "/tmp/archive.zip")).Times(1).WillOnce(Return(scanResult));

    // Needed because of path mismatch
    auto mockFileSystem = new StrictMock<MockFileSystem>{};
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem = std::unique_ptr<IFileSystem>(mockFileSystem);
    EXPECT_CALL(*mockFileSystem, calculateDigest(_, -1)).Times(1).WillOnce(Return("calculatedSha256"));

    EXPECT_CALL(*m_mockThreatReporter, sendThreatReport(_))
        .Times(1)
        .WillOnce(Invoke(
            [&](const scan_messages::ThreatDetected& threatDetected)
            {
                expectCorrectThreatDetected(
                    threatDetected, "/tmp/archive.zip", "threatName_1", "threatType_1", "calculatedSha256", "sha256_1");
            }));

    datatypes::AutoFd autoFd;

    scan_messages::ScanResponse response = susiScanner.scan(autoFd, makeScanRequestObject("/tmp/archive.zip"));

    EXPECT_FALSE(response.allClean());
    ASSERT_EQ(response.getDetections().size(), 2);
    EXPECT_EQ(response.getDetections().at(0).path, "/tmp/archive.zip/eicar1.txt");
    EXPECT_EQ(response.getDetections().at(0).name, "threatName_1");
    EXPECT_EQ(response.getDetections().at(0).sha256, "sha256_1");
    EXPECT_EQ(response.getDetections().at(1).path, "/tmp/archive.zip/eicar2.txt");
    EXPECT_EQ(response.getDetections().at(1).name, "threatName_2");
    EXPECT_EQ(response.getDetections().at(1).sha256, "sha256_2");
    EXPECT_EQ(response.getErrorMsg(), "");

    EXPECT_TRUE(appenderContains("Detected \"threatName_1\" in /tmp/archive.zip/eicar1.txt (On Demand)"));
    EXPECT_TRUE(appenderContains("Detected \"threatName_2\" in /tmp/archive.zip/eicar2.txt (On Demand)"));
    EXPECT_TRUE(appenderContains("Sending report for detection 'threatName_1' in /tmp/archive.zip"));
}

TEST_F(TestSusiScanner, scan_SafeStoreRescanDoesNotSendThreatDetectedReport)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto* mockUnitScanner = new StrictMock<MockUnitScanner>();

    EXPECT_CALL(*m_mockTimer, reset()).Times(1);
    EXPECT_CALL(*m_mockSusiGlobalHandler, isPuaApproved(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockSusiGlobalHandler, isAllowListedPath(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockSusiGlobalHandler, isAllowListedSha256(_)).WillRepeatedly(Return(false));

    ScanResult scanResult{ { { "/tmp/eicar.txt", "threatName", "threatType", "sha256" } }, {} };
    EXPECT_CALL(*mockUnitScanner, scan(_, "/tmp/eicar.txt")).Times(1).WillOnce(Return(scanResult));

    SusiScanner susiScanner{ makeScanner(mockUnitScanner) };

    datatypes::AutoFd autoFd;

    scan_messages::ScanResponse response =
        susiScanner.scan(autoFd, makeScanRequestObject("/tmp/eicar.txt", scan_messages::E_SCAN_TYPE_SAFESTORE_RESCAN));

    EXPECT_FALSE(response.allClean());
    ASSERT_EQ(response.getDetections().size(), 1);
    EXPECT_EQ(response.getDetections().at(0).path, "/tmp/eicar.txt");
    EXPECT_EQ(response.getDetections().at(0).name, "threatName");
    EXPECT_EQ(response.getDetections().at(0).sha256, "sha256");
    EXPECT_EQ(response.getErrorMsg(), "");

    EXPECT_FALSE(appenderContains("Detected"));
    EXPECT_FALSE(appenderContains("Sending report for detection"));
}

TEST_F(TestSusiScanner, archiveNotInSUSIResultCanBeAllowListed)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto* mockUnitScanner = new StrictMock<MockUnitScanner>();

    EXPECT_CALL(*m_mockTimer, reset()).Times(1);
    EXPECT_CALL(*m_mockSusiGlobalHandler, isAllowListedSha256("calculatedSha256")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockSusiGlobalHandler, isAllowListedSha256("sha256_1")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockSusiGlobalHandler, isAllowListedSha256("sha256_2")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockSusiGlobalHandler, isAllowListedPath(_)).WillRepeatedly(Return(false));

    ScanResult scanResult{ { { "/tmp/archive.zip/eicar1.txt", "threatName_1", "threatType_1", "sha256_1" },
                             { "/tmp/archive.zip/eicar2.txt", "threatName_2", "threatType_2", "sha256_2" } },
                           {} };
    EXPECT_CALL(*mockUnitScanner, scan(_, "/tmp/archive.zip")).Times(1).WillOnce(Return(scanResult));

    // Needed because of path mismatch
    auto mockFileSystem = new StrictMock<MockFileSystem>{};
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem = std::unique_ptr<IFileSystem>(mockFileSystem);
    EXPECT_CALL(*mockFileSystem, calculateDigest(_, -1)).Times(1).WillOnce(Return("calculatedSha256"));

    EXPECT_CALL(*m_mockThreatReporter, sendThreatReport(_)).Times(0); // No calls to threatReporter

    SusiScanner susiScanner{ makeScannerWithReporter(mockUnitScanner) };
    datatypes::AutoFd autoFd;
    scan_messages::ScanResponse response = susiScanner.scan(autoFd, makeScanRequestObject("/tmp/archive.zip"));

    EXPECT_TRUE(response.allClean());
    EXPECT_EQ(response.getDetections().size(), 0);
    EXPECT_EQ(response.getErrorMsg(), "");

    EXPECT_TRUE(appenderContains("Allowing /tmp/archive.zip with calculatedSha256"));
}

TEST_F(TestSusiScanner, puaApproved)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    const auto mockUnitScanner = new StrictMock<MockUnitScanner>();
    SusiScanner susiScanner{ makeScannerWithReporter(mockUnitScanner) };
    EXPECT_CALL(*m_mockTimer, reset()).Times(1);
    EXPECT_CALL(*m_mockSusiGlobalHandler, isPuaApproved(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockSusiGlobalHandler, isAllowListedPath(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockSusiGlobalHandler, isAllowListedSha256(_)).WillRepeatedly(Return(false));

    ScanResult scanResult{ { { "/tmp/eicar.txt", "An approved PUA", "PUA", "sha256" } }, {} };
    EXPECT_CALL(*mockUnitScanner, scan(_, "/tmp/eicar.txt")).Times(1).WillOnce(Return(scanResult));

    datatypes::AutoFd autoFd;
    auto scanInfo = makeScanRequestObject("/tmp/eicar.txt");
    scanInfo.setDetectPUAs(true);
    scan_messages::ScanResponse response = susiScanner.scan(autoFd, scanInfo);

    EXPECT_TRUE(response.allClean());
    ASSERT_EQ(response.getDetections().size(), 0);
    EXPECT_EQ(response.getErrorMsg(), "");
    EXPECT_TRUE(appenderContains("Allowing PUA /tmp/eicar.txt by exclusion 'An approved PUA'"));
}

TEST_F(TestSusiScanner, puaNotApproved)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    const auto mockUnitScanner = new StrictMock<MockUnitScanner>();
    SusiScanner susiScanner{ makeScannerWithReporter(mockUnitScanner) };
    EXPECT_CALL(*m_mockTimer, reset()).Times(1);
    EXPECT_CALL(*m_mockSusiGlobalHandler, isPuaApproved(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockSusiGlobalHandler, isAllowListedPath(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockSusiGlobalHandler, isAllowListedSha256(_)).WillRepeatedly(Return(false));

    const std::string puaClassName = "Not approved PUA";
    ScanResult scanResult{ { { "/tmp/eicar.txt", puaClassName, "PUA", "sha256" } }, {} };
    EXPECT_CALL(*mockUnitScanner, scan(_, "/tmp/eicar.txt")).Times(1).WillOnce(Return(scanResult));

    WaitForEvent serverWaitGuard;
    EXPECT_CALL(*m_mockThreatReporter, sendThreatReport(_))
        .Times(1)
        .WillOnce(Invoke(
            [&](const scan_messages::ThreatDetected& threatDetected)
            {
                expectCorrectThreatDetected(threatDetected, "/tmp/eicar.txt", puaClassName, "PUA", "sha256", "sha256");
                EXPECT_EQ(threatDetected.pid, -1);
                serverWaitGuard.onEventNoArgs();
            }));

    datatypes::AutoFd autoFd;
    auto scanInfo = makeScanRequestObject("/tmp/eicar.txt");
    scanInfo.setDetectPUAs(true);
    scan_messages::ScanResponse response = susiScanner.scan(autoFd, scanInfo);

    serverWaitGuard.wait();

    EXPECT_FALSE(response.allClean());
    ASSERT_EQ(response.getDetections().size(), 1);
    EXPECT_EQ(response.getDetections().at(0).path, "/tmp/eicar.txt");
    EXPECT_EQ(response.getDetections().at(0).name, puaClassName);
    EXPECT_EQ(response.getDetections().at(0).sha256, "sha256");
    EXPECT_EQ(response.getErrorMsg(), "");

    EXPECT_TRUE(appenderContains("Detected \"Not approved PUA\" in /tmp/eicar.txt"));
}

TEST_F(TestSusiScanner, puaDetectionDisabled)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    const auto mockUnitScanner = new StrictMock<MockUnitScanner>();
    SusiScanner susiScanner{ makeScannerWithReporter(mockUnitScanner) };
    EXPECT_CALL(*m_mockTimer, reset()).Times(1);
    EXPECT_CALL(*m_mockSusiGlobalHandler, isPuaApproved(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockSusiGlobalHandler, isAllowListedPath(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockSusiGlobalHandler, isAllowListedSha256(_)).WillRepeatedly(Return(false));

    ScanResult scanResult{ { { "/tmp/eicar.txt", "Some PUA", "PUA", "sha256" } }, {} };
    EXPECT_CALL(*mockUnitScanner, scan(_, "/tmp/eicar.txt")).Times(1).WillOnce(Return(scanResult));

    datatypes::AutoFd autoFd;
    auto scanInfo = makeScanRequestObject("/tmp/eicar.txt");
    scanInfo.setDetectPUAs(false);
    scan_messages::ScanResponse response = susiScanner.scan(autoFd, scanInfo);

    EXPECT_TRUE(response.allClean());
    ASSERT_EQ(response.getDetections().size(), 0);
    EXPECT_EQ(response.getErrorMsg(), "");
    EXPECT_TRUE(appenderContains("Allowing /tmp/eicar.txt as PUA detection is disabled"));
}

TEST_F(TestSusiScanner, scan_AllowedByPath)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto* mockUnitScanner = new StrictMock<MockUnitScanner>();

    SusiScanner susiScanner{ makeScanner(mockUnitScanner) };

    EXPECT_CALL(*m_mockTimer, reset()).Times(1);
    EXPECT_CALL(*m_mockSusiGlobalHandler, isAllowListedPath("/tmp/eicar.txt")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockSusiGlobalHandler, isAllowListedSha256(_)).WillRepeatedly(Return(false));

    ScanResult scanResult{ { { "/tmp/eicar.txt", "threatName", "threatType", "sha256" } }, {} };
    EXPECT_CALL(*mockUnitScanner, scan(_, "/tmp/eicar.txt")).Times(1).WillOnce(Return(scanResult));

    datatypes::AutoFd autoFd;

    scan_messages::ScanResponse response = susiScanner.scan(autoFd, makeScanRequestObject("/tmp/eicar.txt"));

    EXPECT_TRUE(appenderContains("Allowing /tmp/eicar.txt as path is in allow list"));
    EXPECT_TRUE(response.allClean());
    EXPECT_EQ(response.getErrorMsg(), "");
}

TEST_F(TestSusiScanner, puaAllowedInScanRequest)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    const auto mockUnitScanner = new StrictMock<MockUnitScanner>();
    SusiScanner susiScanner{ makeScannerWithReporter(mockUnitScanner) };
    EXPECT_CALL(*m_mockTimer, reset()).Times(1);
    EXPECT_CALL(*m_mockSusiGlobalHandler, isPuaApproved(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockSusiGlobalHandler, isAllowListedPath(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockSusiGlobalHandler, isAllowListedSha256(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockThreatReporter, sendThreatReport(_)).Times(0);

    ScanResult scanResult{ { { "/tmp/eicar.txt", "An approved PUA", "PUA", "sha256" } }, {} };
    EXPECT_CALL(*mockUnitScanner, scan(_, "/tmp/eicar.txt")).Times(1).WillOnce(Return(scanResult));

    datatypes::AutoFd autoFd;
    auto scanInfo = makeScanRequestObject("/tmp/eicar.txt");
    scanInfo.setDetectPUAs(true);
    scanInfo.setPuaExclusions({ "An approved PUA" });
    scan_messages::ScanResponse response = susiScanner.scan(autoFd, scanInfo);

    EXPECT_TRUE(response.allClean());
    EXPECT_EQ(response.getDetections().size(), 0);
    EXPECT_EQ(response.getErrorMsg(), "");
    EXPECT_TRUE(appenderContains("Allowing PUA /tmp/eicar.txt by request exclusion 'An approved PUA'"));
    EXPECT_FALSE(appenderContains("Trying to get the main detection when no detections were provided"));
    EXPECT_FALSE(appenderContains("Sending report for detection"));
}

TEST_F(TestSusiScanner, puaNotAllowedInScanRequest)
{
    constexpr auto* puaClassName = "Not approved PUA";

    UsingMemoryAppender memoryAppenderHolder(*this);
    const auto mockUnitScanner = new StrictMock<MockUnitScanner>();
    SusiScanner susiScanner{ makeScannerWithReporter(mockUnitScanner) };
    EXPECT_CALL(*m_mockTimer, reset()).Times(1);
    EXPECT_CALL(*m_mockSusiGlobalHandler, isPuaApproved(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockSusiGlobalHandler, isAllowListedPath(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockSusiGlobalHandler, isAllowListedSha256(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockThreatReporter, sendThreatReport(_)).Times(1);

    ScanResult scanResult{ { { "/tmp/eicar.txt", puaClassName, "PUA", "sha256" } }, {} };
    EXPECT_CALL(*mockUnitScanner, scan(_, "/tmp/eicar.txt")).Times(1).WillOnce(Return(scanResult));

    datatypes::AutoFd autoFd;
    auto scanInfo = makeScanRequestObject("/tmp/eicar.txt");
    scanInfo.setDetectPUAs(true);
    scanInfo.setPuaExclusions({ "An approved PUA" });
    scan_messages::ScanResponse response = susiScanner.scan(autoFd, scanInfo);

    EXPECT_FALSE(response.allClean());
    ASSERT_EQ(response.getDetections().size(), 1);
    EXPECT_EQ(response.getDetections().at(0).path, "/tmp/eicar.txt");
    EXPECT_EQ(response.getDetections().at(0).name, puaClassName);
    EXPECT_EQ(response.getDetections().at(0).sha256, "sha256");
    EXPECT_EQ(response.getErrorMsg(), "");
    EXPECT_FALSE(appenderContains("Allowing PUA /tmp/eicar.txt by request exclusion 'An approved PUA'"));
}

TEST_F(TestSusiScanner, multiDetectionShouldIgnorePUA_1st)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    const auto mockUnitScanner = new StrictMock<MockUnitScanner>();
    SusiScanner susiScanner{ makeScannerWithReporter(mockUnitScanner) };
    EXPECT_CALL(*m_mockTimer, reset()).Times(1);
    EXPECT_CALL(*m_mockSusiGlobalHandler, isPuaApproved(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockSusiGlobalHandler, isAllowListedPath(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockSusiGlobalHandler, isAllowListedSha256(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockThreatReporter, sendThreatReport(_)).Times(1);

    ScanResult scanResult{ {
                               { "/tmp/eicar.txt", "EICAR-PUA", "PUA", "sha256" },
                               { "/tmp/eicar.txt", "EICAR-Virus", "virus", "sha256" },
                           },
                           {} };
    EXPECT_CALL(*mockUnitScanner, scan(_, "/tmp/eicar.txt")).Times(1).WillOnce(Return(scanResult));

    datatypes::AutoFd autoFd;
    auto scanInfo = makeScanRequestObject("/tmp/eicar.txt");
    scanInfo.setDetectPUAs(false);
    scan_messages::ScanResponse response = susiScanner.scan(autoFd, scanInfo);

    EXPECT_FALSE(response.allClean());
    ASSERT_EQ(response.getDetections().size(), 1);
    auto detection = response.getDetections()[0];
    EXPECT_EQ(detection.name, "EICAR-Virus");
    EXPECT_EQ(response.getErrorMsg(), "");
}

TEST_F(TestSusiScanner, multiDetectionShouldIgnorePUA_2nd)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    const auto mockUnitScanner = new StrictMock<MockUnitScanner>();
    SusiScanner susiScanner{ makeScannerWithReporter(mockUnitScanner) };
    EXPECT_CALL(*m_mockTimer, reset()).Times(1);
    EXPECT_CALL(*m_mockSusiGlobalHandler, isPuaApproved(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockSusiGlobalHandler, isAllowListedPath(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockSusiGlobalHandler, isAllowListedSha256(_)).WillRepeatedly(Return(false));

    ScanResult scanResult{ {
                               { "/tmp/eicar.txt", "EICAR-Virus", "virus", "sha256" },
                               { "/tmp/eicar.txt", "EICAR-PUA", "PUA", "sha256" },
                           },
                           {} };
    EXPECT_CALL(*mockUnitScanner, scan(_, "/tmp/eicar.txt")).Times(1).WillOnce(Return(scanResult));

    datatypes::AutoFd autoFd;
    auto scanInfo = makeScanRequestObject("/tmp/eicar.txt");
    scanInfo.setDetectPUAs(false);
    scan_messages::ScanResponse response = susiScanner.scan(autoFd, scanInfo);

    EXPECT_FALSE(response.allClean());
    ASSERT_EQ(response.getDetections().size(), 1);
    auto detection = response.getDetections()[0];
    EXPECT_EQ(detection.name, "EICAR-Virus");
    EXPECT_EQ(response.getErrorMsg(), "");
}

TEST_F(TestSusiScanner, MetadataRescanScansThreatAndFileIfShasDiffer)
{
    EXPECT_CALL(*m_mockTimer, reset()).Times(1);
    const auto mockUnitScanner = new StrictMock<MockUnitScanner>();
    SusiScanner susiScanner{ makeScanner(mockUnitScanner) };

    MetadataRescan request{ "filePath", "sha256", { "threatType", "threatName", "threatSha256" } };

    EXPECT_CALL(*mockUnitScanner, metadataRescan("threatType", "threatName", "filePath", "threatSha256"));
    EXPECT_CALL(*mockUnitScanner, metadataRescan("unknown", "unknown", "filePath", "sha256"));

    susiScanner.metadataRescan(request);
}

TEST_F(TestSusiScanner, MetadataRescanScansThreatAndButNotFileIfShasAreTheSame)
{
    EXPECT_CALL(*m_mockTimer, reset()).Times(1);
    const auto mockUnitScanner = new StrictMock<MockUnitScanner>();
    SusiScanner susiScanner{ makeScanner(mockUnitScanner) };

    MetadataRescan request{ "filePath", "sha256", { "threatType", "threatName", "sha256" } };

    EXPECT_CALL(*mockUnitScanner, metadataRescan).Times(0);
    EXPECT_CALL(*mockUnitScanner, metadataRescan("threatType", "threatName", "filePath", "sha256"));

    susiScanner.metadataRescan(request);
}

namespace
{
    struct TestSusiScannerMetadataRescanDifferentShaParameter
    {
        MetadataRescanResponse fileResponse;
        MetadataRescanResponse threatResponse;
        MetadataRescanResponse result;
    };

    class TestSusiScannerMetadataRescanDifferentShaHarness
        : public TestSusiScanner,
          public testing::WithParamInterface<TestSusiScannerMetadataRescanDifferentShaParameter>
    {
    };
} // namespace

// This is the case in, for example, an archive
TEST_P(TestSusiScannerMetadataRescanDifferentShaHarness, MetadataRescanHasExpectedResult)
{
    EXPECT_CALL(*m_mockTimer, reset()).Times(1);
    const auto mockUnitScanner = new StrictMock<MockUnitScanner>();
    SusiScanner susiScanner{ makeScanner(mockUnitScanner) };

    MetadataRescan request{ "filePath", "sha256", { "threatType", "threatName", "threatSha256" } };

    EXPECT_CALL(*mockUnitScanner, metadataRescan("threatType", "threatName", "filePath", "threatSha256"))
        .WillOnce(Return(GetParam().threatResponse));
    EXPECT_CALL(*mockUnitScanner, metadataRescan("unknown", "unknown", "filePath", "sha256"))
        .WillOnce(Return(GetParam().fileResponse));

    const auto result = susiScanner.metadataRescan(request);
    EXPECT_EQ(result, GetParam().result);
}

INSTANTIATE_TEST_SUITE_P(
    CleanOnWholeFileOverridesResultOnThreat,
    TestSusiScannerMetadataRescanDifferentShaHarness,
    ValuesIn(std::vector<TestSusiScannerMetadataRescanDifferentShaParameter>{
        { MetadataResponse::clean, MetadataResponse::clean, MetadataResponse::clean },
        { MetadataResponse::clean, MetadataResponse::threatPresent, MetadataResponse::clean },
        { MetadataResponse::clean, MetadataResponse::needsFullScan, MetadataResponse::clean },
        { MetadataResponse::clean, MetadataResponse::undetected, MetadataResponse::clean },
        { MetadataResponse::clean, MetadataResponse::failed, MetadataResponse::clean } }));

INSTANTIATE_TEST_SUITE_P(
    ThreatPresentOnWholeFileOverridesResultOnThreat,
    TestSusiScannerMetadataRescanDifferentShaHarness,
    ValuesIn(std::vector<TestSusiScannerMetadataRescanDifferentShaParameter>{
        { MetadataResponse::threatPresent, MetadataResponse::clean, MetadataResponse::threatPresent },
        { MetadataResponse::threatPresent, MetadataResponse::threatPresent, MetadataResponse::threatPresent },
        { MetadataResponse::threatPresent, MetadataResponse::needsFullScan, MetadataResponse::threatPresent },
        { MetadataResponse::threatPresent, MetadataResponse::undetected, MetadataResponse::threatPresent },
        { MetadataResponse::threatPresent, MetadataResponse::failed, MetadataResponse::threatPresent } }));

INSTANTIATE_TEST_SUITE_P(
    NeedsFullScanOnWholeFileOverridesResultOnThreat,
    TestSusiScannerMetadataRescanDifferentShaHarness,
    ValuesIn(std::vector<TestSusiScannerMetadataRescanDifferentShaParameter>{
        { MetadataResponse::needsFullScan, MetadataResponse::clean, MetadataResponse::needsFullScan },
        { MetadataResponse::needsFullScan, MetadataResponse::threatPresent, MetadataResponse::needsFullScan },
        { MetadataResponse::needsFullScan, MetadataResponse::needsFullScan, MetadataResponse::needsFullScan },
        { MetadataResponse::needsFullScan, MetadataResponse::undetected, MetadataResponse::needsFullScan },
        { MetadataResponse::needsFullScan, MetadataResponse::failed, MetadataResponse::needsFullScan } }));

INSTANTIATE_TEST_SUITE_P(
    FailedOnWholeFileOverridesResultOnThreat,
    TestSusiScannerMetadataRescanDifferentShaHarness,
    ValuesIn(std::vector<TestSusiScannerMetadataRescanDifferentShaParameter>{
        { MetadataResponse::failed, MetadataResponse::clean, MetadataResponse::failed },
        { MetadataResponse::failed, MetadataResponse::threatPresent, MetadataResponse::failed },
        { MetadataResponse::failed, MetadataResponse::needsFullScan, MetadataResponse::failed },
        { MetadataResponse::failed, MetadataResponse::undetected, MetadataResponse::failed },
        { MetadataResponse::failed, MetadataResponse::failed, MetadataResponse::failed } }));

INSTANTIATE_TEST_SUITE_P(
    UndetectedOnWholeFileReturnsResultForFile,
    TestSusiScannerMetadataRescanDifferentShaHarness,
    ValuesIn(std::vector<TestSusiScannerMetadataRescanDifferentShaParameter>{
        // This is needsFullScan because it could have other threats that were not rescanned
        { MetadataResponse::undetected, MetadataResponse::clean, MetadataResponse::needsFullScan },
        { MetadataResponse::undetected, MetadataResponse::undetected, MetadataResponse::needsFullScan },
        { MetadataResponse::undetected, MetadataResponse::threatPresent, MetadataResponse::threatPresent },
        { MetadataResponse::undetected, MetadataResponse::needsFullScan, MetadataResponse::needsFullScan },
        { MetadataResponse::undetected, MetadataResponse::failed, MetadataResponse::failed } }));

namespace
{
    struct TestSusiScannerMetadataRescanSameShaParameter
    {
        MetadataRescanResponse threatResponse;
        MetadataRescanResponse result;
    };

    class TestSusiScannerMetadataRescanSameSha
        : public TestSusiScanner,
          public testing::WithParamInterface<TestSusiScannerMetadataRescanSameShaParameter>
    {
    };
} // namespace

TEST_P(TestSusiScannerMetadataRescanSameSha, MetadataRescanHasExpectedResult)
{
    EXPECT_CALL(*m_mockTimer, reset()).Times(1);
    const auto mockUnitScanner = new StrictMock<MockUnitScanner>();
    SusiScanner susiScanner{ makeScanner(mockUnitScanner) };

    MetadataRescan request{ "filePath", "sha256", { "threatType", "threatName", "sha256" } };

    EXPECT_CALL(*mockUnitScanner, metadataRescan("threatType", "threatName", "filePath", "sha256"))
        .WillOnce(Return(GetParam().threatResponse));

    const auto result = susiScanner.metadataRescan(request);
    EXPECT_EQ(result, GetParam().result);
}

INSTANTIATE_TEST_SUITE_P(
    CleanOnFileOverridesThreat,
    TestSusiScannerMetadataRescanSameSha,
    ValuesIn(std::vector<TestSusiScannerMetadataRescanSameShaParameter>{
        // The same SHA256 doesn't necessarily mean it isn't an archive, as the archive could have an 'outer
        // stream' detection so the SHA256s for that threat would match the SHA256 of the archive. But a
        // clean detection here must mean the whole file is considered clean
        { MetadataResponse::clean, MetadataResponse::clean },
        { MetadataResponse::undetected, MetadataResponse::needsFullScan },
        { MetadataResponse::needsFullScan, MetadataResponse::needsFullScan },
        { MetadataResponse::threatPresent, MetadataResponse::threatPresent },
        { MetadataResponse::failed, MetadataResponse::failed } }));
