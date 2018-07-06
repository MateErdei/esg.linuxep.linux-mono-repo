/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <Common/ReactorImpl/ReadableFd.h>
#include <future>
#include "TempDir.h"
#include "TestExecutionSynchronizer.h"
#include "Common/ReactorImpl/GenericCallbackListener.h"
#include "Common/Reactor/IReactor.h"
#include "Common/ReactorImpl/ReactorImpl.h"
#include "MockCallBackListener.h"
#include "PipeForTests.h"
#include "Common/ZeroMQWrapper/IContext.h"
#include "Common/ZeroMQWrapperImpl/ZeroMQWrapperException.h"
#include "FakeServer.h"
#include "FakeClient.h"
#include "Common/Process/IProcess.h"
#include "ReactorImplTestsPath.h"

using namespace Common::Reactor;
using data_t = Common::ZeroMQWrapper::IReadable::data_t;
using namespace Common::ReactorImpl;

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
    data_t processData = {{"hello"}};

    Tests::TestExecutionSynchronizer testExecutionSynchronizer;

    EXPECT_CALL(mockCallBackListener, messageHandler(processData)).WillOnce(
            Invoke([&testExecutionSynchronizer](data_t){testExecutionSynchronizer.notify(); })
    );

    reactor->addListener(&readableFd, &mockCallBackListener);

    reactor->start();

    m_pipe->write("hello");

    testExecutionSynchronizer.waitfor();
    reactor->stop();
}
/**
 * Reactor is tested indirectly via the use of the Fake server to give more real world results.
 */

TEST_F(ReactorImplTest, TestFakeServerCommandsRespondCorrectly)
{
    auto context = Common::ZeroMQWrapper::createContext();

    std::string socketAddress = "inproc://ReactorImplTest_TestFakeServer1";

    FakeClient fakeClient(*context, socketAddress, 200);
    FakeServer fakeServer(socketAddress, false);

    fakeServer.run(*context);

    data_t requestData{"echo", "arg1", "arg2"};
    EXPECT_EQ(fakeClient.requestReply(requestData), requestData );

    data_t requestData2{"concat", "arg1", "arg2"};
    data_t expectedRequestData2{"concat", "arg1arg2"};
    EXPECT_EQ(fakeClient.requestReply(requestData2), expectedRequestData2);

    data_t requestData3{"quit"};
    EXPECT_EQ(fakeClient.requestReply(requestData3), requestData3 );

    // throws if fake server has stopped.
    EXPECT_THROW(fakeClient.requestReply(requestData), Common::ZeroMQWrapperImpl::ZeroMQWrapperException);
}

TEST_F(ReactorImplTest, TestFakeServerSignalHandlerCommandsRespondCorrectly)
{
    Tests::TempDir tempDir("/tmp");

    std::string socketAddress = std::string("ipc://") + tempDir.dirPath() + "/test.ipc";

    auto process = Common::Process::createProcess();
    auto fileSystem = Common::FileSystem::createFileSystem();
    std::string fakeServerPath = fileSystem->join(ReactorImplTestsPath(), "FakeServerRunner");
    ASSERT_TRUE( fileSystem->isExecutable(fakeServerPath));
    data_t args{socketAddress};
    process->exec(fakeServerPath, args);


    auto context = Common::ZeroMQWrapper::createContext();

    FakeClient fakeClient(*context, socketAddress, 5000);

    data_t requestData{"echo", "arg1", "arg2"};
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


    GenericCallbackListener callBackListenerThatThrows([&callbackExecuted](data_t){
        callbackExecuted = true;
        throw std::runtime_error("Test Error");
    });
    MockCallBackListener mockCallBackListener;

    auto reactor = Common::Reactor::createReactor();
    std::unique_ptr<PipeForTests> m_pipeWhichThrows = std::unique_ptr<PipeForTests>(new PipeForTests());
    ReadableFd readableFdThatThrows(m_pipeWhichThrows->readFd(), false);
    ReadableFd readableFd(m_pipe->readFd(), false);
    data_t processData = {{"hello"}};

    Tests::TestExecutionSynchronizer executionSynchronizer;


    EXPECT_CALL(mockCallBackListener, messageHandler(processData)).WillOnce(
            Invoke([&executionSynchronizer](data_t){executionSynchronizer.notify();})
    );

    reactor->addListener(&readableFdThatThrows, &callBackListenerThatThrows);
    reactor->addListener(&readableFd, &mockCallBackListener);

    reactor->start();

    m_pipeWhichThrows->write("hello");
    m_pipe->write("hello");

    executionSynchronizer.waitfor();
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

