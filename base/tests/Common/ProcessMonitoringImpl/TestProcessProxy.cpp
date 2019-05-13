/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/ProcessMonitoring/IProcessProxy.h>
#include <Common/Process/IProcessInfo.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>


TEST(ProcessProxy, createProcessProxyDoesntThrow)  //NOLINT
{
    auto processInfoPtr = Common::Process::createEmptyProcessInfo();
    ASSERT_NO_THROW(Common::ProcessMonitoring::createProcessProxy(std::move(processInfoPtr)));
}