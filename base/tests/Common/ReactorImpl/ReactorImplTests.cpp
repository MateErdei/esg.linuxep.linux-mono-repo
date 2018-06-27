/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <Common/ReactorImpl/ReadableFd.h>
#include <future>
#include "Common/ReactorImpl/GenericCallbackListener.h"
#include "Common/Reactor/IReactor.h"
#include "Common/ReactorImpl/ReactorImpl.h"
#include "MockCallBackListener.h"
#include "MockShutdownListener.h"
#include "PipeForTests.h"
#include "Common/ZeroMQWrapper/IContext.h"
#include "Common/ZeroMQWrapperImpl/ZeroMQWrapperException.h"
#include "FakeServer.h"
#include "FakeClient.h"
#include "Common/Process/IProcess.h"
#include "Common/FileSystem/IFileSystem.h"
#include "TempDir.h"

using namespace Common::Reactor;

class SyncTest
{
    std::mutex mutex;
    std::condition_variable m_waitCondition;
public:
    void waitfor()
    {
        std::unique_lock<std::mutex> lock(mutex);
        m_waitCondition.wait_for(lock, std::chrono::milliseconds(500));
    }
    void notify()
    {
        std::unique_lock<std::mutex> lock(mutex);
        m_waitCondition.notify_all();
    }
};


class ReactorImplTest : public ::testing::Test
{
public:

    void SetUp() override
    {
        m_pipe = std::unique_ptr<PipeForTests>(new PipeForTests());
    }
    void TearDown() override
    {
        m_pipe.reset();
    }

    std::unique_ptr<PipeForTests> m_pipe;
};



TEST_F( ReactorImplTest, AddSingleCallbackListenerAndTestWritingData)
{
    using ::testing::Invoke;
    MockCallBackListener mockCallBackListener;

    auto reactor = Common::Reactor::createReactor();
    ReadableFd readableFd(m_pipe->readFd(), false);
    std::vector<std::string> processData = {{"hello"}};

    SyncTest sync;

    EXPECT_CALL(mockCallBackListener, process(processData)).WillOnce(
            Invoke([&sync](std::vector<std::string>){sync.notify(); })
    );

    reactor->addListener(&readableFd, &mockCallBackListener);

    reactor->start();

    m_pipe->write("hello");

    sync.waitfor();
    reactor->stop();
}

TEST_F(ReactorImplTest, TestFakeServerCommandsRespondCorrectly)
{
    auto context = Common::ZeroMQWrapper::createContext();

    std::string socketAddress = "inproc://ReactorImplTest_TestFakeServer1";

    FakeClient fakeClient(*context, socketAddress, 200);
    FakeServer fakeServer(socketAddress, false);

    fakeServer.run(*context);

    std::vector<std::string> requestData{"echo", "arg1", "arg2"};
    EXPECT_EQ(fakeClient.requestReply(requestData), requestData );

    std::vector<std::string> requestData2{"concat", "arg1", "arg2"};
    std::vector<std::string> expectedRequestData2{"concat", "arg1arg2"};
    EXPECT_EQ(fakeClient.requestReply(requestData2), expectedRequestData2);

    std::vector<std::string> requestData3{"quit"};
    EXPECT_EQ(fakeClient.requestReply(requestData3), requestData3 );

    // throws if fake server has stopped.
    EXPECT_THROW(fakeClient.requestReply(requestData), Common::ZeroMQWrapperImpl::ZeroMQWrapperException);
}

TEST_F(ReactorImplTest, TestFakeServerSignalHandlerCommandsRespondCorrectly)
{
    std::string socketAddress = "ipc:///tmp/TestFakeServerSignalHandlerCommandsRespondCorrectly.ipc";
    auto process = Common::Process::createProcess();
    auto fileSystem = Common::FileSystem::createFileSystem();
    std::string fakeServerPath = fileSystem->join(fileSystem->currentWorkingDirectory(), "FakeServerRunner");
    std::vector<std::string> args{socketAddress};
    process->exec(fakeServerPath, args);


    auto context = Common::ZeroMQWrapper::createContext();

    FakeClient fakeClient(*context, socketAddress, -1);

    std::vector<std::string> requestData{"echo", "arg1", "arg2"};
    EXPECT_EQ(fakeClient.requestReply(requestData), requestData );


    process->kill();

    EXPECT_EQ(0, process->exitCode());
}

TEST_F( ReactorImplTest, CallingStopBeforeStartAndNoListenersDoesNotThrow)
{
    auto reactor = Common::Reactor::createReactor();
    ReadableFd readableFd(m_pipe->readFd(), false);

    EXPECT_NO_THROW(reactor->stop());
}

TEST_F( ReactorImplTest, CallingStartStopWithNoListenersDoesNotThrow)
{
    auto reactor = Common::Reactor::createReactor();
    ReadableFd readableFd(m_pipe->readFd(), false);

    EXPECT_NO_THROW(reactor->start());
    EXPECT_NO_THROW(reactor->stop());
}

TEST_F( ReactorImplTest, CallingStopBeforeStartWithAListenersDoesNotThrow)
{
    MockCallBackListener mockCallBackListener;

    auto reactor = Common::Reactor::createReactor();
    ReadableFd readableFd(m_pipe->readFd(), false);


    reactor->addListener(&readableFd, &mockCallBackListener);

    EXPECT_NO_THROW(reactor->stop());
}


TEST_F( ReactorImplTest, callbackListenerThatThrowsDoesNotPreventOtherListenersFromRunning)
{
    using ::testing::Invoke;
    bool callbackExecuted = false;


    GenericCallbackListener callBackListenerThatThrows([&callbackExecuted](std::vector<std::string>){
        callbackExecuted = true;
        throw std::runtime_error("Test Error");
    });
    MockCallBackListener mockCallBackListener;

    auto reactor = Common::Reactor::createReactor();
    std::unique_ptr<PipeForTests> m_pipeWhichThrows = std::unique_ptr<PipeForTests>(new PipeForTests());
    ReadableFd readableFdThatThrows(m_pipeWhichThrows->readFd(), false);
    ReadableFd readableFd(m_pipe->readFd(), false);
    std::vector<std::string> processData = {{"hello"}};

    SyncTest sync;


    EXPECT_CALL(mockCallBackListener, process(processData)).WillOnce(
            Invoke([&sync](std::vector<std::string>){sync.notify();})
    );

    reactor->addListener(&readableFdThatThrows, &callBackListenerThatThrows);
    reactor->addListener(&readableFd, &mockCallBackListener);

    reactor->start();

    m_pipeWhichThrows->write("hello");
    m_pipe->write("hello");

    sync.waitfor();
    ASSERT_TRUE(callbackExecuted);
    reactor->stop();
}
#ifndef NDEBUG
/**
 * Test fucntion can only be called in debug mode.
 */
TEST_F(ReactorImplTest, addingListenerAfterReactorThreadStartedShouldFailWithAssert )
{
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";

    MockCallBackListener mockCallBackListener;

    auto reactor = Common::Reactor::createReactor();
    ReadableFd readableFd(m_pipe->readFd(), false);

    EXPECT_DEATH({reactor->start();reactor->addListener(&readableFd, &mockCallBackListener);}, "");
}
#endif

