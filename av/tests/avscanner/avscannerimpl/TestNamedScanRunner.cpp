// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "ScanRunnerMemoryAppenderUsingTests.h"

#include "avscanner/avscannerimpl/NamedScanRunner.h"
#include "datatypes/sophos_filesystem.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"

// Test includes
#include "tests/common/MockMountPoint.h"
#include "tests/common/RecordingMockSocket.h"

// 3rd party includes
#include <capnp/message.h>
#include <capnp/serialize.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <fstream>

using namespace avscanner::avscannerimpl;
using namespace mount_monitor::mountinfo;
using namespace testing;

namespace fs = sophos_filesystem;

namespace
{
    class TestNamedScanRunner : public ScanRunnerMemoryAppenderUsingTests
    {
    protected:
        void SetUp() override
        {
            const ::testing::TestInfo* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
            m_testDir = fs::temp_directory_path();
            m_testDir /= test_info->test_case_name();
            m_testDir /= test_info->name();
            fs::remove_all(m_testDir);
            fs::create_directories(m_testDir);

            for(const auto& p: getAllOtherDirs(m_testDir))
            {
                m_expectedExclusions.push_back(p);
            }

            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
            appConfig.setData(Common::ApplicationConfiguration::SOPHOS_INSTALL, m_testDir );
        }

        void TearDown() override
        {
            fs::remove_all(m_testDir);
        }

        static std::vector<std::string> getAllOtherDirs(const std::string& includedDir)
        {
            fs::path currentDir = includedDir;
            std::vector<std::string> allOtherDirs;
            do
            {
                currentDir = currentDir.parent_path();
                for (const auto& p : fs::directory_iterator(currentDir))
                {

                    if (includedDir.rfind(p.path(), 0) != 0)
                    {
                        if (p.status().type() == fs::file_type::directory)
                        {
                            allOtherDirs.push_back(p.path().string() + "/");
                        }
                        else
                        {
                            allOtherDirs.push_back(p.path().string());
                        }
                    }
                }
            } while (currentDir != "/");
            return allOtherDirs;
        }

        Sophos::ssplav::NamedScan::Reader createNamedScanConfig(
                ::capnp::MallocMessageBuilder& message,
                std::vector<std::string> expectedExclusions,
                bool scanHardDisc,
                bool scanNetwork,
                bool scanOptical,
                bool scanRemovable)
        {
            Sophos::ssplav::NamedScan::Builder scanConfigIn = message.initRoot<Sophos::ssplav::NamedScan>();
            scanConfigIn.setName(m_expectedScanName);
            auto exclusions = scanConfigIn.initExcludePaths(expectedExclusions.size());
            for (unsigned i=0; i < expectedExclusions.size(); i++)
            {
                exclusions.set(i, expectedExclusions[i]);
            }
            scanConfigIn.setScanHardDrives(scanHardDisc);
            scanConfigIn.setScanNetworkDrives(scanNetwork);
            scanConfigIn.setScanCDDVDDrives(scanOptical);
            scanConfigIn.setScanRemovableDrives(scanRemovable);

            Sophos::ssplav::NamedScan::Reader scanConfigOut = message.getRoot<Sophos::ssplav::NamedScan>();

            return scanConfigOut;
        }

        Sophos::ssplav::NamedScan::Reader createNamedScanConfig(::capnp::MallocMessageBuilder& message)
        {
            return createNamedScanConfig(
                message,
                m_expectedExclusions,
                m_scanHardDisc,
                m_scanNetwork,
                m_scanOptical,
                m_scanRemovable);
        }

        fs::path m_testDir;
        std::string m_expectedScanName = "testScan";
        std::vector<std::string> m_expectedExclusions;
        bool m_scanHardDisc = true;
        bool m_scanNetwork = false;
        bool m_scanOptical = false;
        bool m_scanRemovable = false;
    };
}

TEST_F(TestNamedScanRunner, TestNamedScanConfigDeserialisation)
{
    m_scanHardDisc = false;
    m_scanNetwork = true;
    m_scanOptical = false;
    m_scanRemovable = true;

    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::NamedScan::Reader scanConfigOut = createNamedScanConfig(
            message,
            m_expectedExclusions,
            m_scanHardDisc,
            m_scanNetwork,
            m_scanOptical,
            m_scanRemovable);

    NamedScanRunner runner(scanConfigOut);

    NamedScanConfig config = runner.getConfig();
    EXPECT_EQ(config.m_scanName, m_expectedScanName);

    std::vector<std::string> excludePaths;
    excludePaths.reserve(config.m_excludePaths.size());
    for(const auto& excl : config.m_excludePaths)
    {
        excludePaths.push_back(excl.path());
    }
    EXPECT_THAT(excludePaths, ContainerEq(m_expectedExclusions));

    EXPECT_EQ(config.m_scanHardDisc, m_scanHardDisc);
    EXPECT_EQ(config.m_scanNetwork, m_scanNetwork);
    EXPECT_EQ(config.m_scanOptical, m_scanOptical);
    EXPECT_EQ(config.m_scanRemovable, m_scanRemovable);
}

TEST_F(TestNamedScanRunner, TestNamedScanConfigDirectoryPassedAsFilename)
{
    try
    {
        NamedScanRunner runner("/tmp");
        FAIL() << "Expected runtime exception";
    }
    catch (const std::runtime_error& e)
    {
        ASSERT_EQ(std::string(e.what()), "Aborting: Config path must not be directory");
    }
}

TEST_F(TestNamedScanRunner, TestNamedScanConfigInvalidFormat)
{
    try
    {
        NamedScanRunner runner("/etc/passwd");
        FAIL() << "Expected runtime exception";
    }
    catch (const std::runtime_error& e)
    {
        ASSERT_EQ(std::string(e.what()), "Aborting: Config file cannot be parsed");
    }
}

TEST_F(TestNamedScanRunner, TestNamedScanConfigBinaryContents)
{
    try
    {
        NamedScanRunner runner("/bin/bash");
        FAIL() << "Expected runtime exception";
    }
    catch (const std::runtime_error& e)
    {
        ASSERT_EQ(std::string(e.what()), "Aborting: Config file cannot be parsed");
    }
}

TEST_F(TestNamedScanRunner, TestNamedScanConfigEmptyFile)
{
    fs::path emptyFile = m_testDir / "TestNamedScanConfigEmptyFile";
    std::ofstream emptyFileHandle(emptyFile);
    try
    {
        NamedScanRunner runner(emptyFile);
        FAIL() << "Expected runtime exception";
    }
    catch (const std::runtime_error& e)
    {
        ASSERT_EQ(std::string(e.what()), "Aborting: Config file cannot be parsed");
    }
}

TEST_F(TestNamedScanRunner, TestNamedScanConfigIncompleteConfig)
{
    std::string filename = "namedScanConfig";
    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::NamedScan::Builder requestBuilder =
        message.initRoot<Sophos::ssplav::NamedScan>();
    requestBuilder.setName(filename);
    requestBuilder.setScanHardDrives(m_scanHardDisc);
    requestBuilder.setScanNetworkDrives(m_scanNetwork);
    requestBuilder.setScanCDDVDDrives(m_scanOptical);
    requestBuilder.setScanRemovableDrives(m_scanRemovable);

    fs::path filePath = m_testDir / filename;
    std::ofstream fileHandle(filePath);
    kj::Array<capnp::word> dataArray = capnp::messageToFlatArray(message);
    kj::ArrayPtr<kj::byte> bytes = dataArray.asBytes();
    std::string dataAsString(bytes.begin(), bytes.end());
    // Write only half of the config to disk
    fileHandle << dataAsString.substr(0, dataAsString.size() / 2);
    fileHandle.close();

    try
    {
        NamedScanRunner runner(filePath);
        FAIL() << "Expected runtime exception";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_EQ(std::string(e.what()), "Aborting: Config file cannot be parsed");
    }
}

TEST_F(TestNamedScanRunner, TestNamedScanConfigNonUTF8fileName)
{
    // echo -n "名前の付いたオンデマンド検索の設定" | iconv -f utf-8 -t euc-jp | hexdump -C
    std::vector<unsigned char> threatPathBytes { 0xcc, 0xbe, 0xc1, 0xb0, 0xa4, 0xce, 0xc9, 0xd5, 0xa4, 0xa4, 0xa4, 0xbf,
                                                 0xa5, 0xaa, 0xa5, 0xf3, 0xa5, 0xc7, 0xa5, 0xde, 0xa5, 0xf3, 0xa5, 0xc9,
                                                 0xb8, 0xa1, 0xba, 0xf7, 0xa4, 0xce, 0xc0, 0xdf, 0xc4, 0xea };
    std::string eucJPfilename(threatPathBytes.begin(), threatPathBytes.end());

    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::NamedScan::Builder requestBuilder =
        message.initRoot<Sophos::ssplav::NamedScan>();
    requestBuilder.setName(eucJPfilename);
    requestBuilder.setScanHardDrives(m_scanHardDisc);
    requestBuilder.setScanNetworkDrives(m_scanNetwork);
    requestBuilder.setScanCDDVDDrives(m_scanOptical);
    requestBuilder.setScanRemovableDrives(m_scanRemovable);

    fs::path eucJPfilePath = m_testDir / eucJPfilename;
    std::ofstream eucJPfileHandle(eucJPfilePath);
    kj::Array<capnp::word> dataArray = capnp::messageToFlatArray(message);
    kj::ArrayPtr<kj::byte> bytes = dataArray.asBytes();
    std::string dataAsString(bytes.begin(), bytes.end());
    eucJPfileHandle << dataAsString;
    eucJPfileHandle.close();

    NamedScanRunner runner(eucJPfilePath);
    NamedScanConfig config = runner.getConfig();
    EXPECT_EQ(config.m_scanName, eucJPfilename);
    EXPECT_EQ(config.m_scanHardDisc, m_scanHardDisc);
    EXPECT_EQ(config.m_scanNetwork, m_scanNetwork);
    EXPECT_EQ(config.m_scanOptical, m_scanOptical);
    EXPECT_EQ(config.m_scanRemovable, m_scanRemovable);
}

TEST_F(TestNamedScanRunner, TestNamedScanConfigLargeFile)
{
    std::string largeScanConfigFilename = "TestNamedScanConfigLargeFile";

    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::NamedScan::Builder requestBuilder =
        message.initRoot<Sophos::ssplav::NamedScan>();
    requestBuilder.setName(largeScanConfigFilename);
    requestBuilder.setScanHardDrives(m_scanHardDisc);
    requestBuilder.setScanNetworkDrives(m_scanNetwork);
    requestBuilder.setScanCDDVDDrives(m_scanOptical);
    requestBuilder.setScanRemovableDrives(m_scanRemovable);

    fs::path largeScanConfigPath = m_testDir / largeScanConfigFilename;
    std::ofstream largeScanConfigHandle(largeScanConfigPath);
    kj::Array<capnp::word> dataArray = capnp::messageToFlatArray(message);
    kj::ArrayPtr<kj::byte> bytes = dataArray.asBytes();
    std::string dataAsString(bytes.begin(), bytes.end());
    // Append data 3x to make the file 3x the expected size
    largeScanConfigHandle << dataAsString << dataAsString << dataAsString;
    largeScanConfigHandle.close();

    NamedScanRunner runner(largeScanConfigPath);
    NamedScanConfig config = runner.getConfig();
    EXPECT_EQ(config.m_scanName, largeScanConfigFilename);
    EXPECT_EQ(config.m_scanHardDisc, m_scanHardDisc);
    EXPECT_EQ(config.m_scanNetwork, m_scanNetwork);
    EXPECT_EQ(config.m_scanOptical, m_scanOptical);
    EXPECT_EQ(config.m_scanRemovable, m_scanRemovable);
}

TEST_F(TestNamedScanRunner, TestGetIncludedMountpoints)
{
    m_scanHardDisc = true;
    m_scanNetwork = true;
    m_scanOptical = true;
    m_scanRemovable = true;

    std::shared_ptr<::testing::StrictMock<MockMountPoint>> localFixedDevice = std::make_shared<::testing::StrictMock<MockMountPoint>>();
    EXPECT_CALL(*localFixedDevice, isHardDisc()).WillOnce(Return(true));
    EXPECT_CALL(*localFixedDevice, mountPoint()).WillRepeatedly(Return("/"));

    std::shared_ptr<::testing::StrictMock<MockMountPoint>> networkDevice = std::make_shared<::testing::StrictMock<MockMountPoint>>();
    EXPECT_CALL(*networkDevice, isHardDisc()).WillOnce(Return(false));
    EXPECT_CALL(*networkDevice, isNetwork()).WillOnce(Return(true));
    EXPECT_CALL(*networkDevice, mountPoint()).WillRepeatedly(Return("/"));

    std::shared_ptr<::testing::StrictMock<MockMountPoint>> opticalDevice = std::make_shared<::testing::StrictMock<MockMountPoint>>();
    EXPECT_CALL(*opticalDevice, isHardDisc()).WillOnce(Return(false));
    EXPECT_CALL(*opticalDevice, isNetwork()).WillOnce(Return(false));
    EXPECT_CALL(*opticalDevice, isOptical()).WillOnce(Return(true));
    EXPECT_CALL(*opticalDevice, mountPoint()).WillRepeatedly(Return("/"));

    std::shared_ptr<::testing::StrictMock<MockMountPoint>> removableDevice = std::make_shared<::testing::StrictMock<MockMountPoint>>();
    EXPECT_CALL(*removableDevice, isHardDisc()).WillOnce(Return(false));
    EXPECT_CALL(*removableDevice, isNetwork()).WillOnce(Return(false));
    EXPECT_CALL(*removableDevice, isOptical()).WillOnce(Return(false));
    EXPECT_CALL(*removableDevice, isRemovable()).WillOnce(Return(true));
    EXPECT_CALL(*removableDevice, mountPoint()).WillRepeatedly(Return("/"));

    std::shared_ptr<::testing::StrictMock<MockMountPoint>> specialDevice = std::make_shared<::testing::StrictMock<MockMountPoint>>();
    EXPECT_CALL(*specialDevice, isHardDisc()).WillOnce(Return(false));
    EXPECT_CALL(*specialDevice, isNetwork()).WillOnce(Return(false));
    EXPECT_CALL(*specialDevice, isOptical()).WillOnce(Return(false));
    EXPECT_CALL(*specialDevice, isRemovable()).WillOnce(Return(false));
    EXPECT_CALL(*specialDevice, isSpecial()).WillOnce(Return(true));

    ::capnp::MallocMessageBuilder message;
    mount_monitor::mountinfo::IMountPointSharedVector allMountpoints;
    allMountpoints.push_back(localFixedDevice);
    allMountpoints.push_back(networkDevice);
    allMountpoints.push_back(opticalDevice);
    allMountpoints.push_back(removableDevice);
    allMountpoints.push_back(specialDevice);

    Sophos::ssplav::NamedScan::Reader scanConfigOut = createNamedScanConfig(
            message,
            m_expectedExclusions,
            m_scanHardDisc,
            m_scanNetwork,
            m_scanOptical,
            m_scanRemovable);

    NamedScanRunner runner(scanConfigOut);

    EXPECT_EQ(runner.getIncludedMountpoints(allMountpoints).size(), 4);
}

TEST_F(TestNamedScanRunner, TestDuplicateMountPointsGetDeduplicated)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    m_scanHardDisc = true;
    m_scanNetwork = true;
    m_scanOptical = true;
    m_scanOptical = true;
    m_scanRemovable = true;

    fs::path testDir = m_testDir / "mount/point";
    fs::create_directories(testDir);

    std::shared_ptr<FakeMountInfo> mountInfo;
    mountInfo.reset(new FakeMountInfo());
    mountInfo->m_mountPoints.emplace_back(
            std::make_shared<FakeMountPoint>(testDir)
    );
    mountInfo->m_mountPoints.emplace_back(
            std::make_shared<FakeMountPoint>(testDir)
    );

    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::NamedScan::Reader scanConfigOut = createNamedScanConfig(
            message,
            m_expectedExclusions,
            m_scanHardDisc,
            m_scanNetwork,
            m_scanOptical,
            m_scanRemovable);

    auto socket = std::make_shared<RecordingMockSocket>();
    NamedScanRunner runner(scanConfigOut);

    runner.setMountInfo(mountInfo);
    runner.setSocket(socket);
    runner.run();

    ASSERT_TRUE(appenderContains("Skipping duplicate mount point: " + testDir.string()));
    fs::remove_all(testDir);
}

TEST_F(TestNamedScanRunner, TestAbortOnlyHappensOnce)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::NamedScan::Reader scanConfigOut = createNamedScanConfig(message);

    NamedScanRunner runner(scanConfigOut);
    auto socket = std::make_shared<AbortingTestSocket>();
    runner.setSocket(socket);
    runner.run();

    EXPECT_EQ(socket->m_abortCount, 1);
    EXPECT_EQ(appenderCount("Deliberate Abort"), 1);
}

TEST_F(TestNamedScanRunner, TestMissingMountpoint)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    m_scanHardDisc = true;
    m_scanNetwork = true;
    m_scanOptical = true;
    m_scanOptical = true;
    m_scanRemovable = true;

    fs::path testDir1 = m_testDir / "mount/point";
    fs::create_directories(testDir1);

    fs::path testfile1 = testDir1 / "file.txt";
    std::ofstream testfileStream1(testfile1.string());
    testfileStream1 << "scan this file";
    testfileStream1.close();

    fs::path testDir2 = m_testDir / "missing/mount";

    fs::path testDir3 = m_testDir / "mount/point3";
    fs::create_directories(testDir3);

    fs::path testfile3 = testDir3 / "file.txt";
    std::ofstream testfileStream3(testfile3.string());
    testfileStream3 << "scan this file";
    testfileStream3.close();

    std::shared_ptr<FakeMountInfo> mountInfo;
    mountInfo.reset(new FakeMountInfo());

    mountInfo->m_mountPoints.emplace_back(
        std::make_shared<FakeMountPoint>(testDir1)
    );
    mountInfo->m_mountPoints.emplace_back(
        std::make_shared<FakeMountPoint>(testDir2)
    );
    mountInfo->m_mountPoints.emplace_back(
        std::make_shared<FakeMountPoint>(testDir3)
    );

    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::NamedScan::Reader scanConfigOut = createNamedScanConfig(
        message,
        m_expectedExclusions,
        m_scanHardDisc,
        m_scanNetwork,
        m_scanOptical,
        m_scanRemovable);

    auto socket = std::make_shared<RecordingMockSocket>(false);
    NamedScanRunner runner(scanConfigOut);

    runner.setMountInfo(mountInfo);
    runner.setSocket(socket);

    ASSERT_EQ(runner.run(), common::E_CLEAN_SUCCESS);

    std::vector<std::string> paths = {
        testfile1,
        testfile3
    };
    EXPECT_THAT(socket->m_paths, ContainerEq(paths));

    std::stringstream expectedErrMsg;
    expectedErrMsg << "Failed to scan \"" << testDir2.string() << "\": file/folder does not exist";
    EXPECT_TRUE(appenderContains(expectedErrMsg.str()));
    EXPECT_TRUE(appenderContains("1 scan error encountered."));
}