// Copyright 2020-2022, Sophos Limited. All rights reserved.

#include "capnp/message.h"
#include "common/Common.h"
#include "common/MemoryAppender.h"
#include "common/WaitForEvent.h"
#include "sophos_threat_detector/sophosthreatdetectorimpl/MockShutdownTimer.h"
#include "sophos_threat_detector/sophosthreatdetectorimpl/MockThreatReporter.h"
#include "sophos_threat_detector/threat_scanner/IUnitScanner.h"
#include "sophos_threat_detector/threat_scanner/SusiResultUtils.h"
#include "sophos_threat_detector/threat_scanner/SusiScanner.h"
#include "sophos_threat_detector/threat_scanner/ThreatDetectedBuilder.h"
#include "scan_messages/ScanRequest.h"

#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <Common/Helpers/MockFileSystem.h>

#include <gmock/gmock.h>

using namespace threat_scanner;
using namespace testing;

namespace
{
    class MockUnitScanner : public IUnitScanner
    {
    public:
        MOCK_METHOD(ScanResult, scan, (datatypes::AutoFd & fd, const std::string& path));
    };

    class TestSusiScanner : public MemoryAppenderUsingTests
    {
    public:
        TestSusiScanner() : MemoryAppenderUsingTests("ThreatScanner") {}
        void SetUp() override
        {
            m_mockThreatReporter = std::make_shared<NiceMock<MockThreatReporter>>();
            m_mockTimer = std::make_shared<StrictMock<MockShutdownTimer>>();
        }

        std::shared_ptr<NiceMock<MockThreatReporter>> m_mockThreatReporter;
        std::shared_ptr<StrictMock<MockShutdownTimer>> m_mockTimer;


        scan_messages::ScanRequest makeScanRequestObject(const std::string path,
                                                         const scan_messages::E_SCAN_TYPE scanType = scan_messages::E_SCAN_TYPE_ON_DEMAND,
                                                         const std::string userId = "root",
                                                         int64_t pid = -1,
                                                         const std::string exepath = "")
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

        void expectCorrectThreatDetected(
            const scan_messages::ThreatDetected& threatDetected,
            const std::string& path,
            const std::string& threatName,
            const std::string& threatType,
            const std::string& sha256,
            const scan_messages::E_SCAN_TYPE  scanType = scan_messages::E_SCAN_TYPE_ON_DEMAND) const
        {
            EXPECT_EQ(threatDetected.userID, "root");
            EXPECT_EQ(threatDetected.threatType, threat_scanner::convertSusiThreatType(threatType));
            EXPECT_EQ(threatDetected.threatName, threatName);
            EXPECT_EQ(threatDetected.scanType, scanType);
            EXPECT_EQ(threatDetected.notificationStatus, scan_messages::E_NOTIFICATION_STATUS_NOT_CLEANUPABLE);
            EXPECT_EQ(threatDetected.filePath, path);
            EXPECT_EQ(threatDetected.actionCode, scan_messages::E_SMT_THREAT_ACTION_NONE);
            EXPECT_EQ(threatDetected.sha256, sha256);
            EXPECT_EQ(threatDetected.threatId, generateThreatId(path, sha256));
            EXPECT_EQ(threatDetected.isRemote, false);
            EXPECT_EQ(threatDetected.reportSource, threat_scanner::getReportSource(threatName));
        }
    };
} // namespace

TEST_F(TestSusiScanner, scan_CleanScan)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const auto mockUnitScanner = new StrictMock<MockUnitScanner>();

    SusiScanner susiScanner{ std::unique_ptr<IUnitScanner>{ mockUnitScanner }, nullptr, m_mockTimer };

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

    SusiScanner susiScanner{ std::unique_ptr<IUnitScanner>{ mockUnitScanner }, m_mockThreatReporter, m_mockTimer };

    EXPECT_CALL(*m_mockTimer, reset()).Times(1);

    ScanResult scanResult{ { { "/tmp/eicar.txt", "threatName", "threatType", "sha256" } }, {} };
    EXPECT_CALL(*mockUnitScanner, scan(_, "/tmp/eicar.txt")).Times(1).WillOnce(Return(scanResult));

    WaitForEvent serverWaitGuard;
    EXPECT_CALL(*m_mockThreatReporter, sendThreatReport(_))
        .Times(1)
        .WillOnce(Invoke(
            [&](const scan_messages::ThreatDetected& threatDetected)
            {
                expectCorrectThreatDetected(threatDetected, "/tmp/eicar.txt", "threatName", "threatType", "sha256");
                EXPECT_EQ(threatDetected.pid,-1);
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

TEST_F(TestSusiScanner, scan_DetectionOnaccessSetPid)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const auto mockUnitScanner = new StrictMock<MockUnitScanner>();

    SusiScanner susiScanner{ std::unique_ptr<IUnitScanner>{ mockUnitScanner }, m_mockThreatReporter, m_mockTimer };

    EXPECT_CALL(*m_mockTimer, reset()).Times(1);

    ScanResult scanResult{ { { "/tmp/eicar.txt", "threatName", "threatType", "sha256" } }, {} };
    EXPECT_CALL(*mockUnitScanner, scan(_, "/tmp/eicar.txt")).Times(1).WillOnce(Return(scanResult));

    WaitForEvent serverWaitGuard;
    EXPECT_CALL(*m_mockThreatReporter, sendThreatReport(_))
        .Times(1)
        .WillOnce(Invoke(
            [&](const scan_messages::ThreatDetected& threatDetected)
            {
                expectCorrectThreatDetected(threatDetected, "/tmp/eicar.txt", "threatName", "threatType", "sha256", scan_messages::E_SCAN_TYPE_ON_ACCESS);
                EXPECT_EQ(threatDetected.pid,100);
                EXPECT_EQ(threatDetected.executablePath , "/random/path/of/parent");
                serverWaitGuard.onEventNoArgs();
            }));

    datatypes::AutoFd autoFd;
    scan_messages::ScanRequest request = makeScanRequestObject("/tmp/eicar.txt",scan_messages::E_SCAN_TYPE_ON_ACCESS,"root",100,"/random/path/of/parent");

    scan_messages::ScanResponse response = susiScanner.scan(autoFd, request);

    serverWaitGuard.wait();


}
TEST_F(TestSusiScanner, scan_DetectionWithErrors_SendsReport)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const auto mockUnitScanner = new StrictMock<MockUnitScanner>();

    SusiScanner susiScanner{ std::unique_ptr<IUnitScanner>{ mockUnitScanner }, m_mockThreatReporter, m_mockTimer };

    EXPECT_CALL(*m_mockTimer, reset()).Times(1);

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
                expectCorrectThreatDetected(threatDetected, "/tmp/eicar.txt", "threatName", "threatType", "sha256");
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

    SusiScanner susiScanner{ std::unique_ptr<IUnitScanner>{ mockUnitScanner }, nullptr, m_mockTimer };

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

    const auto mockUnitScanner = new StrictMock<MockUnitScanner>();

    SusiScanner susiScanner{ std::unique_ptr<IUnitScanner>{ mockUnitScanner }, m_mockThreatReporter, m_mockTimer };

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
                    threatDetected, "/tmp/eicar.txt", "threatName", "threatType", "calculatedSha256");
                serverWaitGuard.onEventNoArgs();
            }));

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

    const auto mockUnitScanner = new StrictMock<MockUnitScanner>();

    SusiScanner susiScanner{ std::unique_ptr<IUnitScanner>{ mockUnitScanner }, m_mockThreatReporter, m_mockTimer };

    EXPECT_CALL(*m_mockTimer, reset()).Times(1);

    ScanResult scanResult{ { { "/tmp/archive.zip", "threatName_archive", "threatType_archive", "sha256_archive" },
                             { "/tmp/archive.zip/eicar1.txt", "threatName_1", "threatType_1", "sha256_1" },
                             { "/tmp/archive.zip/eicar2.txt", "threatName_2", "threatType_2", "sha256_2" } },
                           {} };
    EXPECT_CALL(*mockUnitScanner, scan(_, "/tmp/archive.zip")).Times(1).WillOnce(Return(scanResult));

    WaitForEvent serverWaitGuard;
    EXPECT_CALL(*m_mockThreatReporter, sendThreatReport(_))
        .Times(1)
        .WillOnce(Invoke(
            [&](const scan_messages::ThreatDetected& threatDetected)
            {
                expectCorrectThreatDetected(
                    threatDetected, "/tmp/archive.zip", "threatName_archive", "threatName_archive", "sha256_archive");
                serverWaitGuard.onEventNoArgs();
            }));

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

    const auto mockUnitScanner = new StrictMock<MockUnitScanner>();

    SusiScanner susiScanner{ std::unique_ptr<IUnitScanner>{ mockUnitScanner }, m_mockThreatReporter, m_mockTimer };

    EXPECT_CALL(*m_mockTimer, reset()).Times(1);

    ScanResult scanResult{ { { "/tmp/archive.zip/eicar1.txt", "threatName_1", "threatType_1", "sha256_1" },
                             { "/tmp/archive.zip/eicar2.txt", "threatName_2", "threatType_2", "sha256_2" } },
                           {} };
    EXPECT_CALL(*mockUnitScanner, scan(_, "/tmp/archive.zip")).Times(1).WillOnce(Return(scanResult));

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
                    threatDetected, "/tmp/archive.zip", "threatName_1", "threatName_1", "calculatedSha256");
                serverWaitGuard.onEventNoArgs();
            }));

    datatypes::AutoFd autoFd;

    scan_messages::ScanResponse response = susiScanner.scan(autoFd, makeScanRequestObject("/tmp/archive.zip"));

    serverWaitGuard.wait();

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

    const auto mockUnitScanner = new StrictMock<MockUnitScanner>();

    SusiScanner susiScanner{ std::unique_ptr<IUnitScanner>{ mockUnitScanner }, nullptr, m_mockTimer };

    EXPECT_CALL(*m_mockTimer, reset()).Times(1);

    ScanResult scanResult{ { { "/tmp/eicar.txt", "threatName", "threatType", "sha256" } },
                           {} };
    EXPECT_CALL(*mockUnitScanner, scan(_, "/tmp/eicar.txt")).Times(1).WillOnce(Return(scanResult));

    datatypes::AutoFd autoFd;

    scan_messages::ScanResponse response = susiScanner.scan(autoFd, makeScanRequestObject("/tmp/eicar.txt",scan_messages::E_SCAN_TYPE_SAFESTORE_RESCAN));

    EXPECT_FALSE(response.allClean());
    ASSERT_EQ(response.getDetections().size(), 1);
    EXPECT_EQ(response.getDetections().at(0).path, "/tmp/eicar.txt");
    EXPECT_EQ(response.getDetections().at(0).name, "threatName");
    EXPECT_EQ(response.getDetections().at(0).sha256, "sha256");
    EXPECT_EQ(response.getErrorMsg(), "");

    EXPECT_FALSE(appenderContains("Detected"));
    EXPECT_FALSE(appenderContains("Sending report for detection"));
}