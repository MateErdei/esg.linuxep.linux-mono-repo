/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/ProcessImpl/ProcessImpl.h>
#include <Telemetry/TelemetryImpl/SystemTelemetryCollectorImpl.h>
#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <tests/Common/Helpers/MockProcess.h>

#include <regex>

using ::testing::Return;

namespace {
    const std::string L_lscpulines(
            "Architecture:        x86_64\nCPU op-mode(s):      32-bit, 64-bit\nByte Order:          "
            "Little Endian\nCPU(s):              2\n");
    const std::string L_hostnamectllines(
            "Static hostname: wellie-sspl-dev\n    Virtualization: vmware\n        Operating System: Ubuntu 18.04.2 LTS\n  "
            "      Kernel: Linux 4.15.0-47-generic\n        Architecture: x86-6\n");

    const std::string L_specialCharacters{"\t\n 平仮名\n      Kernel: ひらがな 4.15.0-47-generic\n"};
    const std::string L_dfTLocalLines("Filesystem     Type     1K-blocks     Used Available Use% Mounted on\n"
                                      "udev           devtmpfs   4041952        0   4041952   0% /dev\n"
                                      "tmpfs          tmpfs       814448     1544    812904   1% /run\n"
                                      "/dev/sda1      ext4      41020640 34176708   4730500  88% /\n"
                                      "tmpfs          tmpfs      4072224    35724   4036500   1% /dev/shm\n");

    const std::string L_osNameLines("NAME=\"Ubuntu\"\n"
                             "VERSION=\"18.04.2 LTS (Bionic Beaver)\"\n"
                             "ID=ubuntu\n"
                             "ID_LIKE=debian\n"
                             "PRETTY_NAME=\"Ubuntu 18.04.2 LTS\"\n"
                             "VERSION_ID=\"18.04\"\n"
                             "HOME_URL=\"https://www.ubuntu.com/\"\n"
                             "SUPPORT_URL=\"https://help.ubuntu.com/\"\n"
                             "BUG_REPORT_URL=\"https://bugs.launchpad.net/ubuntu/\"\n"
                             "PRIVACY_POLICY_URL=\"https://www.ubuntu.com/legal/terms-and-policies/privacy-policy\"\n"
                             "VERSION_CODENAME=bionic\n"
                             "UBUNTU_CODENAME=bionic");

    Telemetry::SystemTelemetryConfig multipleKeysTestSystemTelemetryConfig() {
        return {
                {"kernel",
                        Telemetry::systemTelemetryObjectsConfig().at("kernel")},
                {"os-version",
                        Telemetry::systemTelemetryObjectsConfig().at("os-version")},
                {"cpu-cores",
                        Telemetry::systemTelemetryObjectsConfig().at("cpu-cores")}
        };
    }

    Telemetry::SystemTelemetryConfig kernelTelemetryConfig()
    {
        return
        {
            {
                "kernel",
                Telemetry::systemTelemetryObjectsConfig().at("kernel")
            }
        };
    }
    Telemetry::SystemTelemetryConfig osTelemetryConfig() {
        return
        {
            {"os-name",
             Telemetry::systemTelemetryObjectsConfig().at("os-name")}
        };
    }
    Telemetry::SystemTelemetryConfig lscpuTelemetryConfig()
    {
        return
        {
            {"cpu-cores",
             Telemetry::systemTelemetryObjectsConfig().at("cpu-cores")}
        };
    }

} // namespace

class SystemTelemetryCollectorImplTests : public ::testing::Test
{
public:
    void TearDown() override { Common::ProcessImpl::ProcessFactory::instance().restoreCreator(); }

    void setupMockProcesses(size_t numberOfMockProcesses)
    {
        ASSERT_TRUE(numberOfMockProcesses > 0);
        mockProcessIndex_ = 0;

        for (size_t i = 0; i < numberOfMockProcesses; ++i)
        {
            mockProcesses_.emplace_back(new MockProcess());
        }

        Common::ProcessImpl::ProcessFactory::instance().replaceCreator([this]() {
            return (mockProcessIndex_ == mockProcesses_.size()) ? nullptr
                                                                : std::move(mockProcesses_[mockProcessIndex_++]);
        });
    }

    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
    std::vector<std::unique_ptr<MockProcess>> mockProcesses_;

private:
    size_t mockProcessIndex_ = 0;
};

// These tests assume that keys in SystemTelemetryConfig objects are iterated over in dictionary order.

TEST_F(SystemTelemetryCollectorImplTests, CollectObjectsIntValueOK) // NOLINT
{
    setupMockProcesses(lscpuTelemetryConfig().size());
    auto& mockProcess_ = mockProcesses_[0];

    Telemetry::SystemTelemetryCollectorImpl systemTelemetryCollectorImpl(lscpuTelemetryConfig(), {});

    EXPECT_CALL(*mockProcess_, exec(_, _));
    EXPECT_CALL(*mockProcess_, setOutputLimit(_));
    EXPECT_CALL(*mockProcess_, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
    EXPECT_CALL(*mockProcess_, output()).WillOnce(Return(L_lscpulines));
    EXPECT_CALL(*mockProcess_, exitCode()).WillRepeatedly(Return(EXIT_SUCCESS));

    auto intValue = systemTelemetryCollectorImpl.collectObjects();
    auto cpuCores = intValue.find("cpu-cores");
    ASSERT_NE(cpuCores, intValue.cend());
    ASSERT_EQ(std::get<int>(cpuCores->second[0].second), 2);
}

TEST_F(SystemTelemetryCollectorImplTests, CollectObjectsInvalidIntValue) // NOLINT
{
    setupMockProcesses(lscpuTelemetryConfig().size());
    auto& mockProcess_ = mockProcesses_[0];

    Telemetry::SystemTelemetryCollectorImpl systemTelemetryCollectorImpl(lscpuTelemetryConfig(), {});
    std::string invalidLscpulines(
        "Architecture:        x86_64\nCPU op-mode(s):      32-bit, 64-bit\nByte Order:          "
        "Little Endian\nCPU(s):              two\n");

    EXPECT_CALL(*mockProcess_, exec(_, _));
    EXPECT_CALL(*mockProcess_, setOutputLimit(_));
    EXPECT_CALL(*mockProcess_, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
    EXPECT_CALL(*mockProcess_, output()).WillOnce(Return(invalidLscpulines));
    EXPECT_CALL(*mockProcess_, exitCode()).WillRepeatedly(Return(EXIT_SUCCESS));

    auto intValue = systemTelemetryCollectorImpl.collectObjects();
    ASSERT_EQ(intValue.find("cpu-cores"), intValue.end());
}

TEST_F(SystemTelemetryCollectorImplTests, CollectObjectsTooLargeIntValue) // NOLINT
{
    setupMockProcesses(lscpuTelemetryConfig().size());
    auto& mockProcess_ = mockProcesses_[0];

    Telemetry::SystemTelemetryCollectorImpl systemTelemetryCollectorImpl(lscpuTelemetryConfig(), {});
    std::string invalidLscpulines(
        "Architecture:        x86_64\nCPU op-mode(s):      32-bit, 64-bit\nByte Order:          "
        "Little Endian\nCPU(s): 9999999999999999999999999999999999999\n");

    EXPECT_CALL(*mockProcess_, exec(_, _));
    EXPECT_CALL(*mockProcess_, setOutputLimit(_));
    EXPECT_CALL(*mockProcess_, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
    EXPECT_CALL(*mockProcess_, output()).WillOnce(Return(invalidLscpulines));
    EXPECT_CALL(*mockProcess_, exitCode()).WillRepeatedly(Return(EXIT_SUCCESS));

    auto intValue = systemTelemetryCollectorImpl.collectObjects();
    ASSERT_EQ(intValue.find("cpu-cores"), intValue.end());
}

TEST_F(SystemTelemetryCollectorImplTests, CollectObjectsStringValueOK) // NOLINT
{
    setupMockProcesses(osTelemetryConfig().size());
    auto& mockProcess_ = mockProcesses_[0];

    Telemetry::SystemTelemetryCollectorImpl systemTelemetryCollectorImpl(osTelemetryConfig(), {});

    EXPECT_CALL(*mockProcess_, exec(_, _));
    EXPECT_CALL(*mockProcess_, setOutputLimit(_));
    EXPECT_CALL(*mockProcess_, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
    EXPECT_CALL(*mockProcess_, output()).WillOnce(Return(L_osNameLines));
    EXPECT_CALL(*mockProcess_, exitCode()).WillOnce(Return(EXIT_SUCCESS));

    auto stringValue = systemTelemetryCollectorImpl.collectObjects();

    auto osPretty = stringValue.find("os-name");
    ASSERT_NE(osPretty, stringValue.cend());
    ASSERT_EQ(std::get<std::string>(osPretty->second[0].second), "Ubuntu");
}

TEST_F(SystemTelemetryCollectorImplTests, CollectObjectsCachesCommandOutputMultipleValues) // NOLINT
{
    // hostnamectl command is ran only once and second check uses cache
    setupMockProcesses(3);
    Telemetry::SystemTelemetryCollectorImpl systemTelemetryCollectorImpl(multipleKeysTestSystemTelemetryConfig(), {});
    for (auto& mockProcess : mockProcesses_)
    {
        EXPECT_CALL(*mockProcess, exec(_, _));
        EXPECT_CALL(*mockProcess, setOutputLimit(_));
        EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));

        EXPECT_CALL(*mockProcess, output()).WillOnce(Return(L_hostnamectllines + L_lscpulines + L_osNameLines));

        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(EXIT_SUCCESS));
    }

    auto multiValues = systemTelemetryCollectorImpl.collectObjects();
    ASSERT_EQ(multiValues.size(), multipleKeysTestSystemTelemetryConfig().size());
}

TEST_F(SystemTelemetryCollectorImplTests, CollectObjectsCommandReturnsSpecialChars) // NOLINT
{
    setupMockProcesses(kernelTelemetryConfig().size());
    auto& mockProcess = mockProcesses_[0];
    Telemetry::SystemTelemetryCollectorImpl systemTelemetryCollectorImpl(kernelTelemetryConfig(), {});

    EXPECT_CALL(*mockProcess, exec(_, _));
    EXPECT_CALL(*mockProcess, setOutputLimit(_));
    EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
    EXPECT_CALL(*mockProcess, output()).WillOnce(Return(L_specialCharacters));
    EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(EXIT_SUCCESS));

    auto stringValue = systemTelemetryCollectorImpl.collectObjects();

    auto kernel = stringValue.find("kernel");
    ASSERT_NE(kernel, stringValue.cend());
    ASSERT_EQ(std::get<0>(kernel->second[0].second), "ひらがな 4.15.0-47-generic");
}

TEST_F(SystemTelemetryCollectorImplTests, CollectObjectsCommandReturnsEmptyString) // NOLINT
{
    setupMockProcesses(kernelTelemetryConfig().size());
    auto& mockProcess = mockProcesses_[0];
    Telemetry::SystemTelemetryCollectorImpl systemTelemetryCollectorImpl(kernelTelemetryConfig(), {});

    EXPECT_CALL(*mockProcess, exec(_, _));
    EXPECT_CALL(*mockProcess, setOutputLimit(_));
    EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
    EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
    EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(EXIT_SUCCESS));

    auto stringValue = systemTelemetryCollectorImpl.collectObjects();
    ASSERT_EQ(stringValue.empty(), true);
}

TEST_F(SystemTelemetryCollectorImplTests, CollectObjectsProcessImplExitCodeIsFailure) // NOLINT
{
    setupMockProcesses(kernelTelemetryConfig().size());
    auto& mockProcess = mockProcesses_[0];
    Telemetry::SystemTelemetryCollectorImpl systemTelemetryCollectorImpl(kernelTelemetryConfig(), {});

    EXPECT_CALL(*mockProcess, exec(_, _));
    EXPECT_CALL(*mockProcess, setOutputLimit(_));
    EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
    EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(EXIT_FAILURE));

    auto stringValue = systemTelemetryCollectorImpl.collectObjects();
    ASSERT_EQ(stringValue.empty(), true);
}

TEST_F(SystemTelemetryCollectorImplTests, CollectObjectsProcessImplMultipleWithTimeout) // NOLINT
{
    // Mock systemctl call.
    setupMockProcesses(multipleKeysTestSystemTelemetryConfig().size());
    Telemetry::SystemTelemetryCollectorImpl systemTelemetryCollectorImpl(multipleKeysTestSystemTelemetryConfig(), {});
    InSequence s;
    { // mockProcesses_[0]
        EXPECT_CALL(*mockProcesses_[0], setOutputLimit(_));
        EXPECT_CALL(*mockProcesses_[0], exec(_, _));
        EXPECT_CALL(*mockProcesses_[0], wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
        EXPECT_CALL(*mockProcesses_[0], exitCode()).WillOnce(Return(EXIT_SUCCESS));
        EXPECT_CALL(*mockProcesses_[0], output()).WillOnce(Return(L_lscpulines));
    }

    { // mockProcesses_[1]
        EXPECT_CALL(*mockProcesses_[1], setOutputLimit(_));
        EXPECT_CALL(*mockProcesses_[1], exec(_, _));
        EXPECT_CALL(*mockProcesses_[1], wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::TIMEOUT));
        EXPECT_CALL(*mockProcesses_[1], kill()).WillOnce(Return(false));
    }

    { // mockProcesses_[2]
        EXPECT_CALL(*mockProcesses_[2], setOutputLimit(_));
        EXPECT_CALL(*mockProcesses_[2], exec(_, _));
        EXPECT_CALL(*mockProcesses_[2], wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));

        EXPECT_CALL(*mockProcesses_[2], exitCode()).WillOnce(Return(EXIT_SUCCESS));
        EXPECT_CALL(*mockProcesses_[2], output()).WillOnce(Return(L_osNameLines));
    }
    auto multiValues = systemTelemetryCollectorImpl.collectObjects();
    ASSERT_EQ(multiValues.size(), 2);
}

TEST_F(SystemTelemetryCollectorImplTests, CollectObjectsProcessImplMultipleWithExitCodeFail) // NOLINT
{
    // Mock systemctl call.
    setupMockProcesses(multipleKeysTestSystemTelemetryConfig().size());
    Telemetry::SystemTelemetryCollectorImpl systemTelemetryCollectorImpl(multipleKeysTestSystemTelemetryConfig(), {});
    InSequence s;
    { // mockProcesses_[0]
        EXPECT_CALL(*mockProcesses_[0], setOutputLimit(_));
        EXPECT_CALL(*mockProcesses_[0], exec(_, _));
        EXPECT_CALL(*mockProcesses_[0], wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));

        EXPECT_CALL(*mockProcesses_[0], exitCode()).WillOnce(Return(EXIT_SUCCESS));
        EXPECT_CALL(*mockProcesses_[0], output()).WillOnce(Return(L_lscpulines));
    }

    { // mockProcesses_[1]
        EXPECT_CALL(*mockProcesses_[1], setOutputLimit(_));
        EXPECT_CALL(*mockProcesses_[1], exec(_, _));
        EXPECT_CALL(*mockProcesses_[1], wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
        EXPECT_CALL(*mockProcesses_[1], exitCode()).WillOnce(Return(EXIT_FAILURE));
    }

    { // mockProcesses_[2]
        EXPECT_CALL(*mockProcesses_[2], setOutputLimit(_));
        EXPECT_CALL(*mockProcesses_[2], exec(_, _));
        EXPECT_CALL(*mockProcesses_[2], wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));

        EXPECT_CALL(*mockProcesses_[2], exitCode()).WillOnce(Return(EXIT_SUCCESS));
        EXPECT_CALL(*mockProcesses_[2], output()).WillOnce(Return(L_osNameLines));
    }
    auto multiValues = systemTelemetryCollectorImpl.collectObjects();
    ASSERT_EQ(multiValues.size(), 2);
}

TEST_F(SystemTelemetryCollectorImplTests, CollectArrayObjectStringIntValuesOK) // NOLINT
{
    Telemetry::SystemTelemetryConfig multiLineTelemetryConfig = {
            {"disks",
             Telemetry::systemTelemetryArraysConfig().at("disks")}
    };
    setupMockProcesses(multiLineTelemetryConfig.size());
    auto& mockProcess_ = mockProcesses_[0];

    Telemetry::SystemTelemetryCollectorImpl systemTelemetryCollectorImpl(
        multiLineTelemetryConfig, multiLineTelemetryConfig);

    EXPECT_CALL(*mockProcess_, exec(_, _));
    EXPECT_CALL(*mockProcess_, setOutputLimit(_));
    EXPECT_CALL(*mockProcess_, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
    EXPECT_CALL(*mockProcess_, output()).WillOnce(Return(L_dfTLocalLines));
    EXPECT_CALL(*mockProcess_, exitCode()).WillRepeatedly(Return(EXIT_SUCCESS));

    auto mapOfArrayItems = systemTelemetryCollectorImpl.collectArraysOfObjects();

    auto disks = mapOfArrayItems.find("disks");
    ASSERT_NE(disks, mapOfArrayItems.cend());
    auto diskItemArray = disks->second;
    ASSERT_EQ(diskItemArray.size(), 4);
    ASSERT_EQ(diskItemArray[0].size(), 2);

    // verify first elemet in L_dfTLocalLines
    // "udev           devtmpfs   4041952        0   4041952   0% /dev\n"
    auto fstype = std::get<std::string>((diskItemArray[0])[0].second);
    ASSERT_EQ(fstype, std::string("devtmpfs"));
    ASSERT_EQ(std::get<int>((diskItemArray[0])[1].second), 4041952);
}

TEST_F(SystemTelemetryCollectorImplTests, CollectArrayObjectInvalidSubmerges) // NOLINT
{
    Telemetry::SystemTelemetryConfig multiLineTestTelemetryConfig = {
            {"disks",
             Telemetry::systemTelemetryArraysConfig().at("disks")},
             { "Test",
               { "/usr/bin/test",
            { "--testarg", "--testxargs" },
            "^(CPU.*): .*$",
            { { "no-show1", Telemetry::TelemetryValueType::STRING },
              { "no-show2", Telemetry::TelemetryValueType::INTEGER } } } }
    };

    setupMockProcesses(multiLineTestTelemetryConfig.size());

    Telemetry::SystemTelemetryCollectorImpl systemTelemetryCollectorImpl({}, multiLineTestTelemetryConfig);

    for (auto& mockProcess : mockProcesses_)
    {
        EXPECT_CALL(*mockProcess, exec(_, _));
        EXPECT_CALL(*mockProcess, setOutputLimit(_));
        EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
        EXPECT_CALL(*mockProcess, exitCode()).WillRepeatedly(Return(EXIT_SUCCESS));
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return(L_dfTLocalLines));
    }

    auto mapOfArrayItems = systemTelemetryCollectorImpl.collectArraysOfObjects();

    auto disks = mapOfArrayItems.find("disks");
    ASSERT_NE(disks, mapOfArrayItems.cend());
    auto diskItemArray = disks->second;
    ASSERT_EQ(diskItemArray.size(), 4);
    ASSERT_EQ(diskItemArray[0].size(), 2);

    // verify first elemet in L_dfTLocalLines
    // "udev           devtmpfs   4041952        0   4041952   0% /dev\n"
    auto fstype = std::get<std::string>((diskItemArray[0])[0].second);
    ASSERT_EQ(fstype, std::string("devtmpfs"));
    ASSERT_EQ(std::get<int>((diskItemArray[0])[1].second), 4041952);
}


TEST_F(SystemTelemetryCollectorImplTests, CollectObjectsSystemCommandExeNotFound) // NOLINT
{
    Telemetry::SystemTelemetryConfig unavailableSysCommandConfig = {
            { "cpu-cores",
              { "non_existant_system_command", {}, R"(^CPU\(s\): (.*)$)", { { "", Telemetry::TelemetryValueType::INTEGER } } } }
    };
    setupMockProcesses(unavailableSysCommandConfig.size());
    auto& mockProcess_ = mockProcesses_[0];
    Telemetry::SystemTelemetryCollectorImpl systemTelemetryCollectorImpl(unavailableSysCommandConfig, {});

    EXPECT_CALL(*mockProcess_, setOutputLimit(_));

    auto telemetryValues = systemTelemetryCollectorImpl.collectObjects();
    ASSERT_TRUE(telemetryValues.empty());
}

TEST_F(SystemTelemetryCollectorImplTests, CollectObjectsSelinuxOk) // NOLINT
{
    Telemetry::SystemTelemetryConfig selinuxStatusTelemetryConfig = {
            {
                    "selinux", Telemetry::systemTelemetryObjectsConfig().at("selinux")
            }
    };
    std::string enforcementLevel("Disabled" );
    setupMockProcesses(selinuxStatusTelemetryConfig.size());
    auto& mockProcess_ = mockProcesses_[0];

    MockFileSystem* mockFileSystem = nullptr;
    std::unique_ptr<MockFileSystem> mockfileSystem(new StrictMock<MockFileSystem>());
    mockFileSystem = mockfileSystem.get();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockfileSystem));

    Telemetry::SystemTelemetryCollectorImpl systemTelemetryCollectorImpl(selinuxStatusTelemetryConfig, {});

    EXPECT_CALL(*mockFileSystem, isExecutable(_)).WillOnce(Return(true));
    EXPECT_CALL(*mockProcess_, exec(_, _));
    EXPECT_CALL(*mockProcess_, setOutputLimit(_));
    EXPECT_CALL(*mockProcess_, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
    EXPECT_CALL(*mockProcess_, output()).WillOnce(Return(enforcementLevel));
    EXPECT_CALL(*mockProcess_, exitCode()).WillOnce(Return(EXIT_SUCCESS));

    auto stringValue = systemTelemetryCollectorImpl.collectObjects();
    auto selinuxStatus = stringValue.find("selinux");
    ASSERT_NE(selinuxStatus, stringValue.cend());
    ASSERT_EQ(std::get<std::string>(selinuxStatus->second[0].second), enforcementLevel);
}
