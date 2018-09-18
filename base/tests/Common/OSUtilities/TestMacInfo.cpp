/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Common/OSUtilities/MACinfo.h>
#include <Common/ProcessImpl/ProcessImpl.h>
#include <Common/Process/IProcess.h>
#include <Common/FileSystem/IFileSystem.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <iostream>

using namespace Common::OSUtilities;
using PairResult = std::pair<std::string , std::string >;
using ListInputOutput = std::vector<PairResult >;


TEST(TestMacInfo, MacsShouldBeAvailableInIfconfig) // NOLINT
{
    auto fSystem = Common::FileSystem::fileSystem();
    if( !fSystem->isExecutable("/sbin/ifconfig") )
    {
        std::cout << "[  SKIPPED ] /sbin/ifconfig not present " << std::endl;
        return;
    }
    auto process = Common::Process::createProcess();
    process->exec("/sbin/ifconfig", std::vector<std::string>());
    std::string ifconfigOutput = process->output();

    std::vector<std::string> macs = sortedSystemMACs();
    for( auto & mac : macs)
    {
        EXPECT_THAT( ifconfigOutput, ::testing::HasSubstr(mac));
    }

}
