/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Common/FileSystem/IFileSystem.h>
#include <Common/OSUtilitiesImpl/MACinfo.h>
#include <Common/Process/IProcess.h>
#include <Common/ProcessImpl/ProcessImpl.h>
#include <tests/Common/Helpers/LogInitializedTests.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <iostream>

using namespace Common::OSUtilitiesImpl;
using PairResult = std::pair<std::string, std::string>;
using ListInputOutput = std::vector<PairResult>;

class TestMacInfo: public LogOffInitializedTests{};


TEST_F(TestMacInfo, stringfyMACShouldKeepTwoCharactersPerByte) // NOLINT
{
    Common::OSUtilitiesImpl::MACType macType = { 0, 0xf5, 0x43, 0x54, 0xd5, 0x00 };
    EXPECT_EQ(Common::OSUtilitiesImpl::stringfyMAC(macType), "00:f5:43:54:d5:00");
}

TEST_F(TestMacInfo, MacsShouldBeAvailableInIfconfig) // NOLINT
{
    auto fSystem = Common::FileSystem::fileSystem();
    std::string ipconfigInfo = "/sbin/ifconfig";
    std::vector<std::string> arguments;

    if (!fSystem->isExecutable(ipconfigInfo))
    {
        ipconfigInfo = "/usr/sbin/ip";
        arguments.push_back("address");
        if (!fSystem->isExecutable(ipconfigInfo))
        {
            std::cout << "[  SKIPPED ] /sbin/ifconfig or /usr/sbin/ip not present " << std::endl;
            return;
        }
    }
    auto process = Common::Process::createProcess();
    process->exec(ipconfigInfo, arguments);
    std::string ifconfigOutput = process->output();

    std::vector<std::string> macs = sortedSystemMACs();
    for (auto& mac : macs)
    {
        std::string fullmacword = " " + mac + " ";
        EXPECT_THAT(ifconfigOutput, ::testing::HasSubstr(fullmacword));
    }
}
