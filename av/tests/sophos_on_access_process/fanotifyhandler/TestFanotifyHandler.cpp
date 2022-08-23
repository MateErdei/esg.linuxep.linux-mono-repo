//Copyright 2022, Sophos Limited.  All rights reserved.

#include "FanotifyHandlerMemoryAppenderUsingTests.h"

#include "datatypes/MockSysCalls.h"
#include "sophos_on_access_process/fanotifyhandler/FanotifyHandler.h"

#include <gtest/gtest.h>

#include <sstream>

using namespace ::testing;

class TestFANotifyHandler : public FanotifyHandlerMemoryAppenderUsingTests
{
protected:
    void SetUp() override
    {
        m_mockSysCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    }

    std::shared_ptr<StrictMock<MockSystemCallWrapper>> m_mockSysCallWrapper;
};

TEST_F(TestFANotifyHandler, construction)
{
    int fanotifyFd = 0;
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_init(FAN_CLOEXEC | FAN_CLASS_CONTENT, O_RDONLY | O_CLOEXEC | O_LARGEFILE)).WillOnce(Return(fanotifyFd));

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(m_mockSysCallWrapper);
    EXPECT_EQ(handler.getFd(), fanotifyFd);

    EXPECT_TRUE(waitForLog("FANotify successfully initialised"));
    std::stringstream logMsg;
    logMsg << "FANotify FD set to " << fanotifyFd;
    EXPECT_TRUE(waitForLog(logMsg.str()));
    EXPECT_FALSE(appenderContains("Unable to initialise fanotify:"));
}

TEST_F(TestFANotifyHandler, construction_logsErrorIfFanotifyInitFails)
{
    int fanotifyFd = -1;
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_init(FAN_CLOEXEC | FAN_CLASS_CONTENT, O_RDONLY | O_CLOEXEC | O_LARGEFILE)).WillOnce(Return(fanotifyFd));

    EXPECT_THROW(sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(m_mockSysCallWrapper), std::runtime_error);
}
