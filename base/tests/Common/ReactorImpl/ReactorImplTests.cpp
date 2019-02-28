/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "FakeClient.h"
#include "FakeServer.h"
#include "MockCallBackListener.h"
#include "PipeForTests.h"
#include "ReactorImplTestsPath.h"

#include "Common/Process/IProcess.h"
#include "Common/Reactor/IReactor.h"
#include "Common/ReactorImpl/GenericCallbackListener.h"
#include "Common/ReactorImpl/ReactorImpl.h"
#include "modules/Common/ZMQWrapperApi/IContext.h"
#include "Common/ZeroMQWrapperImpl/ZeroMQWrapperException.h"

#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/ReactorImpl/ReadableFd.h>
#include <Common/ZeroMQWrapper/ISocketReplier.h>
#include <Common/ZeroMQWrapper/ISocketRequester.h>
#include <Common/ZeroMQWrapperImpl/SocketImpl.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/Common/Helpers/TempDir.h>
#include <tests/Common/Helpers/TestExecutionSynchronizer.h>

#include <future>
#include <zmq.h>

using namespace Common::Reactor;
using data_t = Common::ZeroMQWrapper::IReadable::data_t;
using namespace Common::ReactorImpl;

class ReactorImplTest : public ::testing::Test
{
public:
    void SetUp() override { m_pipe = std::unique_ptr<PipeForTests>(new PipeForTests()); }
    void TearDown() override { m_pipe.reset(); }

    std::unique_ptr<PipeForTests> m_pipe;
    Common::Logging::ConsoleLoggingSetup m_consoleLogging;
};

TEST_F(ReactorImplTest, AddSingleCallbackListenerAndTestWritingData) // NOLINT
{
    using ::testing::Invoke;
    MockCallBackListener mockCallBackListener;

    ReadableFd readableFd(m_pipe->readFd(), false);
    auto reactor = Common::Reactor::createReactor();

    data_t processData = { { "hello" } };

    Tests::TestExecutionSynchronizer testExecutionSynchronizer;

    EXPECT_CALL(mockCallBackListener, messageHandler(processData))
        .WillOnce(Invoke([&testExecutionSynchronizer](data_t) { testExecutionSynchronizer.notify(); }));

    reactor->addListener(&readableFd, &mockCallBackListener);

    reactor->start();

    m_pipe->write("hello");

    testExecutionSynchronizer.waitfor();
    reactor->stop();
}
/**
 * Reactor is tested indirectly via the use of the Fake server to give more real world results.
 */

TEST_F(ReactorImplTest, TestFakeServerCommandsRespondCorrectly) // NOLINT
{
    auto context = Common::ZMQWrapperApi::createContext();

    std::string socketAddress = "inproc://ReactorImplTest_TestFakeServer1";

    FakeClient fakeClient(*context, socketAddress, 200);
    FakeServer fakeServer(socketAddress, false);

    fakeServer.run(*context);

    data_t requestData{ "echo", "arg1", "arg2" };
    EXPECT_EQ(fakeClient.requestReply(requestData), requestData);

    data_t requestData2{ "concat", "arg1", "arg2" };
    data_t expectedRequestData2{ "concat", "arg1arg2" };
    EXPECT_EQ(fakeClient.requestReply(requestData2), expectedRequestData2);

    data_t requestData3{ "quit" };
    EXPECT_EQ(fakeClient.requestReply(requestData3), requestData3);

    // throws if fake server has stopped.
    EXPECT_THROW(fakeClient.requestReply(requestData), Common::ZeroMQWrapperImpl::ZeroMQWrapperException); // NOLINT
}

TEST_F(ReactorImplTest, TestFakeServerSignalHandlerCommandsRespondCorrectly) // NOLINT
{
    Tests::TempDir tempDir("/tmp");

    std::string socketAddress = std::string("ipc://") + tempDir.dirPath() + "/test.ipc";

    auto process = Common::Process::createProcess();
    auto fileSystem = Common::FileSystem::fileSystem();
    std::string fakeServerPath = Common::FileSystem::join(ReactorImplTestsPath(), "FakeServerRunner");
    ASSERT_TRUE(fileSystem->isExecutable(fakeServerPath));
    data_t args{ socketAddress };
    process->exec(fakeServerPath, args);

    auto context = Common::ZMQWrapperApi::createContext();

    FakeClient fakeClient(*context, socketAddress, 5000);

    data_t requestData{ "echo", "arg1", "arg2" };
    EXPECT_EQ(fakeClient.requestReply(requestData), requestData);

    bool required_kill = process->kill();

    if (!required_kill)
    {
        // Only expect a 0 if we managed to terminate the process
        EXPECT_EQ(0, process->exitCode());
    }
}

TEST_F(ReactorImplTest, CallingStopBeforeStartAndNoListenersDoesNotThrow) // NOLINT
{
    auto reactor = Common::Reactor::createReactor();
    ReadableFd readableFd(m_pipe->readFd(), false);

    EXPECT_NO_THROW(reactor->stop()); // NOLINT
}

TEST_F(ReactorImplTest, CallingStartStopWithNoListenersDoesNotThrow) // NOLINT
{
    auto reactor = Common::Reactor::createReactor();
    ReadableFd readableFd(m_pipe->readFd(), false);

    EXPECT_NO_THROW(reactor->start()); // NOLINT
    EXPECT_NO_THROW(reactor->stop());  // NOLINT
}

TEST_F(ReactorImplTest, CallingStopBeforeStartWithAListenersDoesNotThrow) // NOLINT
{
    MockCallBackListener mockCallBackListener;

    auto reactor = Common::Reactor::createReactor();
    ReadableFd readableFd(m_pipe->readFd(), false);

    reactor->addListener(&readableFd, &mockCallBackListener);

    EXPECT_NO_THROW(reactor->stop()); // NOLINT
}

TEST_F(ReactorImplTest, callbackListenerThatThrowsDoesNotPreventOtherListenersFromRunning) // NOLINT
{
    using ::testing::Invoke;
    bool callbackExecuted = false;

    GenericCallbackListener callBackListenerThatThrows([&callbackExecuted](data_t) {
        callbackExecuted = true;
        throw std::runtime_error("Test Error");
    });
    MockCallBackListener mockCallBackListener;

    std::unique_ptr<PipeForTests> m_pipeWhichThrows = std::unique_ptr<PipeForTests>(new PipeForTests());
    ReadableFd readableFdThatThrows(m_pipeWhichThrows->readFd(), false);
    ReadableFd readableFd(m_pipe->readFd(), false);
    auto reactor = Common::Reactor::createReactor();
    data_t processData = { { "hello" } };

    Tests::TestExecutionSynchronizer executionSynchronizer;

    EXPECT_CALL(mockCallBackListener, messageHandler(processData)).WillOnce(Invoke([&executionSynchronizer](data_t) {
        executionSynchronizer.notify();
    }));

    reactor->addListener(&readableFdThatThrows, &callBackListenerThatThrows);
    reactor->addListener(&readableFd, &mockCallBackListener);

    reactor->start();

    m_pipeWhichThrows->write("hello");
    m_pipe->write("hello");

    executionSynchronizer.waitfor();
    ASSERT_TRUE(callbackExecuted);
    reactor->stop();
}

TEST_F(ReactorImplTest, ReactorCallTerminatesIfThePollerBreaks) // NOLINT
{
    auto lambdaThatClosesPipeBeforeStopingReactor = []() {
        using ::testing::Invoke;
        MockCallBackListener mockCallBackListener;
        std::unique_ptr<PipeForTests> pipe(new PipeForTests());
        ReadableFd readableFd(pipe->readFd(), false);
        auto reactor = Common::Reactor::createReactor();

        reactor->addListener(&readableFd, &mockCallBackListener);

        reactor->start();

        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        pipe.reset();
        reactor->join();
    };

    ASSERT_DEATH(lambdaThatClosesPipeBeforeStopingReactor(), "Error associated with the poller"); // NOLINT
}

TEST_F(ReactorImplTest, ReactorCallTerminatesIfThePollerBreaksForZMQSockets) // NOLINT
{
    auto lambdaThatClosesSocketBeforeStopingReactor = []() {
        using ::testing::Invoke;
        auto context = Common::ZMQWrapperApi::createContext();
        auto replier = context->getReplier();
        auto requester = context->getRequester();
        requester->setTimeout(200);
        requester->connect("inproc://REPSocketNotified");

        replier->setTimeout(100);
        replier->listen("inproc://REPSocketNotified");
        MockCallBackListener mockCallBackListener;
        auto reactor = Common::Reactor::createReactor();
        reactor->addListener(replier.get(), &mockCallBackListener);

        reactor->start();

        std::this_thread::sleep_for(std::chrono::milliseconds(30));

        // closes the socket to generate the error in the poller.
        auto socket = dynamic_cast<Common::ZeroMQWrapperImpl::SocketImpl*>(replier.get());
        ASSERT_TRUE(socket != nullptr);
        auto zmqsocket = socket->skt();
        ASSERT_EQ(zmq_close(zmqsocket), 0);

        requester->write({ "hello1" });
        auto fut = std::async(std::launch::async, [&reactor]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            reactor->stop();
        });
        reactor->join();
        fut.get();
    };

    ASSERT_DEATH(lambdaThatClosesSocketBeforeStopingReactor(), "Error associated with the poller"); // NOLINT
}

#ifndef NDEBUG
/**
 * Test fucntion can only be called in debug mode.
 */
TEST_F(ReactorImplTest, addingListenerAfterReactorThreadStartedShouldFailWithAssert) // NOLINT
{
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";

    MockCallBackListener mockCallBackListener;

    auto reactor = Common::Reactor::createReactor();
    ReadableFd readableFd(m_pipe->readFd(), false);

    EXPECT_DEATH(
        {
            reactor->start();
            reactor->addListener(&readableFd, &mockCallBackListener);
        },
        ""); // NOLINT
}
#endif
