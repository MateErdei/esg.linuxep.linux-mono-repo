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
    class TestNamedScanRunnerExclusions : public ScanRunnerMemoryAppenderUsingTests
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

TEST_F(TestNamedScanRunnerExclusions, TestExclusionsAppliedAtInclusionTime)
{
    m_scanHardDisc = true;
    m_scanNetwork = true;
    m_scanOptical = true;
    m_scanRemovable = true;

    std::shared_ptr<::testing::StrictMock<MockMountPoint>> localFixedDevice = std::make_shared<::testing::StrictMock<MockMountPoint>>();
    EXPECT_CALL(*localFixedDevice, isHardDisc()).WillOnce(Return(true));
    EXPECT_CALL(*localFixedDevice, mountPoint()).WillRepeatedly(Return("/etc/a"));

    std::shared_ptr<::testing::StrictMock<MockMountPoint>> networkDevice = std::make_shared<::testing::StrictMock<MockMountPoint>>();
    EXPECT_CALL(*networkDevice, isHardDisc()).WillOnce(Return(false));
    EXPECT_CALL(*networkDevice, isNetwork()).WillOnce(Return(true));
    EXPECT_CALL(*networkDevice, mountPoint()).WillRepeatedly(Return("/etc/b"));

    std::shared_ptr<::testing::StrictMock<MockMountPoint>> opticalDevice = std::make_shared<::testing::StrictMock<MockMountPoint>>();
    EXPECT_CALL(*opticalDevice, isHardDisc()).WillOnce(Return(false));
    EXPECT_CALL(*opticalDevice, isNetwork()).WillOnce(Return(false));
    EXPECT_CALL(*opticalDevice, isOptical()).WillOnce(Return(true));
    EXPECT_CALL(*opticalDevice, mountPoint()).WillRepeatedly(Return("/etc/c"));

    std::shared_ptr<::testing::StrictMock<MockMountPoint>> removableDevice = std::make_shared<::testing::StrictMock<MockMountPoint>>();
    EXPECT_CALL(*removableDevice, isHardDisc()).WillOnce(Return(false));
    EXPECT_CALL(*removableDevice, isNetwork()).WillOnce(Return(false));
    EXPECT_CALL(*removableDevice, isOptical()).WillOnce(Return(false));
    EXPECT_CALL(*removableDevice, isRemovable()).WillOnce(Return(true));
    EXPECT_CALL(*removableDevice, mountPoint()).WillRepeatedly(Return("/home/a"));

    ::capnp::MallocMessageBuilder message;
    mount_monitor::mountinfo::IMountPointSharedVector allMountpoints;
    allMountpoints.push_back(localFixedDevice);
    allMountpoints.push_back(networkDevice);
    allMountpoints.push_back(opticalDevice);
    allMountpoints.push_back(removableDevice);

    Sophos::ssplav::NamedScan::Reader scanConfigOut = createNamedScanConfig(
        message,
        m_expectedExclusions,
        m_scanHardDisc,
        m_scanNetwork,
        m_scanOptical,
        m_scanRemovable);

    NamedScanRunner runner(scanConfigOut);

    EXPECT_EQ(runner.getIncludedMountpoints(allMountpoints).size(), 0);
}

TEST_F(TestNamedScanRunnerExclusions, TestExcludeByStem)
{
    fs::path testfile = m_testDir / "file1.txt";
    std::ofstream testfileStream(testfile.string());
    testfileStream << "this file will be included, then excluded by stem";

    std::string stemExclusion = m_testDir.parent_path().string() + "/";

    ::capnp::MallocMessageBuilder message;
    m_expectedScanName = "TestExcludeByStemScan1";
    Sophos::ssplav::NamedScan::Reader scanConfigOut = createNamedScanConfig(
            message,
            m_expectedExclusions,
            m_scanHardDisc,
            m_scanNetwork,
            m_scanOptical,
            m_scanRemovable);

    NamedScanRunner runner(scanConfigOut);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    EXPECT_THAT(socket->m_paths, Contains(testfile));

    m_expectedExclusions.push_back(stemExclusion);
    Sophos::ssplav::NamedScan::Reader scanConfigOut2 = createNamedScanConfig(
            message,
            m_expectedExclusions,
            m_scanHardDisc,
            m_scanNetwork,
            m_scanOptical,
            m_scanRemovable);

    NamedScanRunner runner2(scanConfigOut2);

    auto socket2 = std::make_shared<RecordingMockSocket>();
    runner2.setSocket(socket2);
    runner2.run();

    EXPECT_THAT(socket2->m_paths, Not(Contains(testfile)));
}

TEST_F(TestNamedScanRunnerExclusions, TestExcludeByFullPath)
{
    fs::path testDir = m_testDir;
    fs::path fullPathExcludedFile = testDir / "foo";
    fs::path fullPathIncludedFile = testDir / "foobar";

    std::ofstream excludedFile(fullPathExcludedFile);
    excludedFile << "This file will be excluded from the scan.";
    std::ofstream includedFile(fullPathIncludedFile);
    includedFile << "This file will be included in the scan.";

    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::NamedScan::Reader scanConfigOut = createNamedScanConfig(
            message,
            m_expectedExclusions,
            m_scanHardDisc,
            m_scanNetwork,
            m_scanOptical,
            m_scanRemovable);

    NamedScanRunner runner(scanConfigOut);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    EXPECT_THAT(socket->m_paths, Contains(fullPathExcludedFile));
    EXPECT_THAT(socket->m_paths, Contains(fullPathIncludedFile));

    m_expectedExclusions.push_back(fullPathExcludedFile);
    Sophos::ssplav::NamedScan::Reader scanConfigOut2 = createNamedScanConfig(
            message,
            m_expectedExclusions,
            m_scanHardDisc,
            m_scanNetwork,
            m_scanOptical,
            m_scanRemovable);

    NamedScanRunner runner2(scanConfigOut2);

    auto socket2 = std::make_shared<RecordingMockSocket>();
    runner2.setSocket(socket2);
    runner2.run();

    EXPECT_THAT(socket2->m_paths, Not(Contains(fullPathExcludedFile)));
    EXPECT_THAT(socket2->m_paths, Contains(fullPathIncludedFile));
}

TEST_F(TestNamedScanRunnerExclusions, TestExcludeByGlob)
{
    fs::path testDir = m_testDir;
    fs::path globExcludedFile = testDir / "foo.txt";
    fs::path globIncludedFile = testDir / "foo.log";
    fs::path globExcludedFile2 = testDir / "foo.1";
    fs::path globIncludedFile2 = testDir / "foo.";

    std::ofstream excludedFile(globExcludedFile);
    excludedFile << "This file will be excluded from the scan.";
    std::ofstream includedFile(globIncludedFile);
    includedFile << "This file will be included in the scan.";
    std::ofstream excludedFile2(globExcludedFile2);
    excludedFile2 << "This file will be excluded from the scan.";
    std::ofstream includedFile2(globIncludedFile2);
    includedFile2 << "This file will be included in the scan.";

    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::NamedScan::Reader scanConfigOut = createNamedScanConfig(
            message,
            m_expectedExclusions,
            m_scanHardDisc,
            m_scanNetwork,
            m_scanOptical,
            m_scanRemovable);

    NamedScanRunner runner(scanConfigOut);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    EXPECT_THAT(socket->m_paths, AnyOf(Contains(globExcludedFile), Contains(globExcludedFile2)));
    EXPECT_THAT(socket->m_paths, AnyOf(Contains(globIncludedFile), Contains(globIncludedFile2)));

    m_expectedExclusions.push_back(testDir / "*.txt");
    m_expectedExclusions.push_back(testDir / "foo.?");
    Sophos::ssplav::NamedScan::Reader scanConfigOut2 = createNamedScanConfig(
            message,
            m_expectedExclusions,
            m_scanHardDisc,
            m_scanNetwork,
            m_scanOptical,
            m_scanRemovable);

    NamedScanRunner runner2(scanConfigOut2);

    auto socket2 = std::make_shared<RecordingMockSocket>();
    runner2.setSocket(socket2);
    runner2.run();

    EXPECT_THAT(socket2->m_paths, Not(AnyOf(Contains(globExcludedFile), Contains(globExcludedFile2))));
    EXPECT_THAT(socket2->m_paths, AnyOf(Contains(globIncludedFile), Contains(globIncludedFile2)));

}

TEST_F(TestNamedScanRunnerExclusions, TestExcludeByFilename)
{
    fs::path testDir = m_testDir;
    fs::path filenameExcludedFile = testDir / "bar" / "foo";
    fs::path filenameIncludedFile = testDir / "foo" / "bar";

    fs::create_directory(testDir / "foo");
    fs::create_directory(testDir / "bar");
    std::ofstream excludedFile(filenameExcludedFile);
    excludedFile << "This file will be excluded from the scan.";
    std::ofstream includedFile(filenameIncludedFile);
    includedFile << "This file will be included in the scan.";

    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::NamedScan::Reader scanConfigOut = createNamedScanConfig(
            message,
            m_expectedExclusions,
            m_scanHardDisc,
            m_scanNetwork,
            m_scanOptical,
            m_scanRemovable);

    NamedScanRunner runner(scanConfigOut);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    EXPECT_THAT(socket->m_paths, Contains(filenameExcludedFile));
    EXPECT_THAT(socket->m_paths, Contains(filenameIncludedFile));

    m_expectedExclusions.emplace_back("foo");
    Sophos::ssplav::NamedScan::Reader scanConfigOut2 = createNamedScanConfig(
            message,
            m_expectedExclusions,
            m_scanHardDisc,
            m_scanNetwork,
            m_scanOptical,
            m_scanRemovable);

    NamedScanRunner runner2(scanConfigOut2);

    auto socket2 = std::make_shared<RecordingMockSocket>();
    runner2.setSocket(socket2);
    runner2.run();

    EXPECT_THAT(socket2->m_paths, Not(Contains(filenameExcludedFile)));
    EXPECT_THAT(socket2->m_paths, Contains(filenameIncludedFile));

}

TEST_F(TestNamedScanRunnerExclusions, TestNamedScanRunnerWithFileNameExclusions)
{
    fs::path testfile = m_testDir / "file1.txt";
    std::ofstream testfileStream(testfile.string());
    testfileStream << "this file will included";

    m_expectedExclusions.emplace_back(m_testDir.parent_path());

    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::NamedScan::Reader scanConfigOut = createNamedScanConfig(
            message,
            m_expectedExclusions,
            m_scanHardDisc,
            m_scanNetwork,
            m_scanOptical,
            m_scanRemovable);

    NamedScanRunner runner(scanConfigOut);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    EXPECT_THAT(socket->m_paths, Contains(testfile));
}

TEST_F(TestNamedScanRunnerExclusions, TestNamedScanRunnerWithNonUTF8Exclusions)
{
    //echo -n "検索から除外するファイル" | iconv -f utf-8 -t euc-jp | hexdump -C
    std::vector<unsigned char> eucJpFileBytes { 0xb8, 0xa1, 0xba, 0xf7, 0xa4, 0xab, 0xa4, 0xe9,
                                                0xbd, 0xfc, 0xb3, 0xb0, 0xa4, 0xb9, 0xa4, 0xeb,
                                                0xa5, 0xd5, 0xa5, 0xa1, 0xa5, 0xa4, 0xa5, 0xeb };
    std::string eucJPfilename(eucJpFileBytes.begin(), eucJpFileBytes.end());

    fs::path eucJPfilepath = m_testDir / eucJPfilename;
    std::ofstream eucJPfileHandle(eucJPfilepath.string());
    eucJPfileHandle << "this file will be excluded";
    eucJPfileHandle.close();

    std::string utf8filename = "検索から除外するファイル";
    fs::path utf8filepath = m_testDir / utf8filename;
    std::ofstream utf8fileHandle(utf8filepath.string());
    utf8fileHandle << "this file will be included";
    utf8fileHandle.close();

    m_expectedExclusions.emplace_back(eucJPfilepath);

    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::NamedScan::Reader scanConfigOut = createNamedScanConfig(
        message,
        m_expectedExclusions,
        m_scanHardDisc,
        m_scanNetwork,
        m_scanOptical,
        m_scanRemovable);

    NamedScanRunner runner(scanConfigOut);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    EXPECT_THAT(socket->m_paths, Not(Contains(eucJPfilepath)));
    EXPECT_THAT(socket->m_paths, Contains(utf8filepath));
}

TEST_F(TestNamedScanRunnerExclusions, TestNamedScanRunnerStopsAtExcludedDirectory)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::path testfile = m_testDir / "file1.txt";
    std::ofstream testfileStream(testfile.string());
    testfileStream << "this file will included";

    fs::path testdir = m_testDir / "dir1";
    fs::create_directory(testdir);

    m_expectedExclusions.emplace_back(m_testDir);
    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::NamedScan::Reader scanConfigOut = createNamedScanConfig(
            message, m_expectedExclusions,
            m_scanHardDisc,
            m_scanNetwork,
            m_scanOptical,
            m_scanRemovable);

    NamedScanRunner runner(scanConfigOut);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    EXPECT_FALSE(appenderContains("Excluding file: " + testfile.string()));
    EXPECT_FALSE(appenderContains("Excluding directory: " + testdir.string()));
}