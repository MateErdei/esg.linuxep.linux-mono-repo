/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef ARTISANBUILD

#    include "ReqRepTestImplementations.h"
#    include <Common/Process/IProcess.h>
#    include <Common/Process/IProcessException.h>
#    include <Common/ProcessImpl/ProcessInfo.h>
#    include <Common/ZMQWrapperApi/IContext.h>
#    include <Common/ZeroMQWrapper/IIPCTimeoutException.h>
#    include <Common/ZeroMQWrapper/IReadable.h>
#    include <Common/ZeroMQWrapper/ISocketPublisher.h>
#    include <Common/ZeroMQWrapper/ISocketReplierPtr.h>
#    include <Common/ZeroMQWrapper/ISocketRequesterPtr.h>
#    include <Common/ZeroMQWrapper/ISocketSubscriber.h>
#    include <Common/ZeroMQWrapperImpl/SocketReplierImpl.h>
#    include <Common/ZeroMQWrapperImpl/SocketRequesterImpl.h>
#    include <Common/ZeroMQWrapperImpl/SocketSubscriberImpl.h>
#    include <gmock/gmock.h>
#    include <gtest/gtest.h>


#    include <tests/Common/Helpers/TempDir.h>
#    include <tests/Common/Helpers/TestExecutionSynchronizer.h>


#    include <future>
#    include <stdio.h>
#    include <stdlib.h>
#    include <thread>
#    include <unistd.h>
#    include <zmq.h>
#include <Common/Process/IProcess.h>

extern char** environ;

#    define PRINT(_X) std::cerr << _X << std::endl

namespace
{
    using ::testing::NiceMock;
    using ::testing::StrictMock;
    using namespace ReqRepTest;

    class TestContext
    {
        Tests::TempDir m_tempDir;

    public:
        TestContext() : m_tempDir("/tmp", "reliab") {}
        std::string killChannel() const { return "ipc://" + m_tempDir.absPath("killchannel.ipc"); }
        std::string serverAddress() const { return "ipc://" + m_tempDir.absPath("server.ipc"); }
    };

    // in order to get a better simulation, run the requester in another process.
    class RunInExternalProcess
    {
        std::string m_killchannel;
        Common::ZMQWrapperApi::IContextSharedPtr m_zmq_context;

        void monitorChild(int pid)
        {
            auto begin = std::chrono::high_resolution_clock::now();
            try
            {
                auto replierKillChannel = m_zmq_context->getReplier();
                replierKillChannel->setTimeout(2000);
                replierKillChannel->listen(m_killchannel);
                data_t request = replierKillChannel->read();
                if (request.at(0) == "killme")
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    ::kill(pid, SIGTERM);
                }
            }
            catch (std::exception& ex)
            {
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
                std::cout << "monitor child: " << ex.what() << " after " << duration << "ns" << std::endl;
            }
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
            int status = 1;
            pid_t exitedPID = ::waitpid(pid, &status, 0);
            assert(exitedPID == pid);
            static_cast<void>(exitedPID);
            std::cerr << "Child PID=" << pid << " exited with " << status << " after " << duration << "ns" << std::endl;
        }

    public:
        RunInExternalProcess(const TestContext& context, Common::ZMQWrapperApi::IContextSharedPtr zmq_context) :
            m_killchannel(context.killChannel()),
            m_zmq_context(std::move(zmq_context))
        {
        }

        void runExec(std::vector<std::string> args)
        {
            // Find executable
            std::string exe = CMAKE_CURRENT_BINARY_DIR;
            exe += "/TestReqRepTool";

            auto process = Common::Process::createProcess();

            process->exec(exe, args);
            auto result = std::make_tuple(process->exitCode(), process->output());
            std::cout << "TestReqRepTool executable output: "<< std::get<1>(result)<<std::endl;
            int exitCode = std::get<0>(result);

            if (exitCode != 0)
            {
                std::cout << "TestReqRepTool executable exit code: " << exitCode << std::endl;
            }
        }
    };

    class ReqRepReliabilityTests : public ::testing::Test
    {
    public:
        TestContext m_testContext;
        ReqRepReliabilityTests() = default;
    };

    TEST_F(ReqRepReliabilityTests, normalReqReplyShouldWork) // NOLINT
    {
        auto zmq_context = createContext();
        RunInExternalProcess runInExternalProcess(m_testContext, zmq_context);
        std::string serveraddress = m_testContext.serverAddress();
        std::string killch = m_testContext.killChannel();

        auto futureRequester = std::async(std::launch::async, [serveraddress, zmq_context]() {
            Requester requester(serveraddress, zmq_context);
            return requester.sendReceive("hello");
        });

        runInExternalProcess.runExec({ serveraddress, killch, "UnreliableReplier", "serveRequest" });

        std::string actual("");

        try {
            // this throws.  "No Incomming data"?
            actual = futureRequester.get();
        }
        catch(std::exception& e)
        {
            std::string s = e.what();
            std::cout << s;
        }

        EXPECT_EQ(std::string("granted"), actual);

    }

    TEST_F(ReqRepReliabilityTests, normalReqReplyShouldWorkUsingReply) // NOLINT
    {
        auto zmq_context = createContext();
        RunInExternalProcess runInExternalProcess(m_testContext, zmq_context);
        std::string serveraddress = m_testContext.serverAddress();
        std::string killch = m_testContext.killChannel();

        auto futureReplier = std::async(std::launch::async, [serveraddress, zmq_context]() {
            Replier replier(serveraddress, zmq_context);
            replier.serveRequest();
        });
        runInExternalProcess.runExec({ serveraddress, killch, "UnreliableRequester", "sendReceive", "hello" });
        EXPECT_NO_THROW(futureReplier.get()); // NOLINT
    }

    TEST_F(ReqRepReliabilityTests, requesterShouldRecoverAReplierFailure) // NOLINT
    {
        auto zmq_context = createContext();
        RunInExternalProcess runInExternalProcess(m_testContext, zmq_context);
        std::string serveraddress = m_testContext.serverAddress();
        std::string killch = m_testContext.killChannel();

        auto futureRequester = std::async(std::launch::async, [serveraddress, zmq_context]() {
            Requester requester(serveraddress, zmq_context);
            EXPECT_THROW(requester.sendReceive("hello1"), Common::ZeroMQWrapper::IIPCTimeoutException); // NOLINT

            int retryCount = 5;
            std::string result;

            while(retryCount > 0)
            {
                try
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    result = requester.sendReceive("hello2");
                    break;
                }
                catch(...)
                {
                    retryCount--;
                }
            }
            return result;
        });

        runInExternalProcess.runExec({ serveraddress, killch, "UnreliableReplier", "breakAfterReceiveMessage"});
        //        runInExternalProcess.runFork(
        //                [serveraddress, killch](){UnreliableReplier ur(serveraddress, killch); ur.serveRequest(); }
        //        );

        runInExternalProcess.runExec({ serveraddress, killch, "UnreliableReplier", "serveRequest"});


        EXPECT_EQ(std::string("granted"), futureRequester.get());

    }

    TEST_F(ReqRepReliabilityTests, requesterShouldRecoverAReplierSendingBrokenMessage) // NOLINT
    {
        auto zmq_context = createContext();
        RunInExternalProcess runInExternalProcess(m_testContext, zmq_context);
        std::string serveraddress = m_testContext.serverAddress();
        std::string killch = m_testContext.killChannel();

        auto futureRequester = std::async(std::launch::async, [serveraddress, zmq_context]() {
            Requester requester(serveraddress, zmq_context);
            EXPECT_THROW(requester.sendReceive("hello1"), Common::ZeroMQWrapper::IIPCTimeoutException); // NOLINT

            int retryCount = 5;
            std::string result;

            while(retryCount > 0)
            {
                try
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    result = requester.sendReceive("hello2");
                    break;
                }
                catch(...)
                {
                    retryCount--;
                }
            }
            return result;
        });

        //        runInExternalProcess.runFork(
        //                [serveraddress, killch](){UnreliableReplier ur(serveraddress, killch); ur.servePartialReply();
        //                }
        //
        //        );

        runInExternalProcess.runExec({ serveraddress, killch, "UnreliableReplier", "servePartialReply" });

        //        runInExternalProcess.runFork(
        //                [serveraddress, killch](){UnreliableReplier ur(serveraddress, killch); ur.serveRequest(); }
        //        );

        runInExternalProcess.runExec({ serveraddress, killch, "UnreliableReplier", "serveRequest" });

        EXPECT_EQ(std::string("granted"), futureRequester.get());
    }

    TEST_F(ReqRepReliabilityTests, replierShouldNotBreakIfRequesterFails) // NOLINT
    {
        auto zmq_context = createContext();
        RunInExternalProcess runInExternalProcess(m_testContext, zmq_context);
        std::string serveraddress = m_testContext.serverAddress();
        std::string killch = m_testContext.killChannel();

        auto futureReplier = std::async(std::launch::async, [serveraddress, zmq_context]() {
            Replier replier(serveraddress, zmq_context, 10000);
            try
            {
                replier.serveRequest();
            }
            catch (std::exception& ex)
            {
                std::cerr << "There was exception for replierShouldNotBreakIfRequesterFails 1: " << ex.what()
                          << std::endl;
                // the first one may or may not throw, but the second must not throw.
            }
            PRINT("Served first request by " << getpid());

            try
            {
                replier.serveRequest();
            }
            catch (std::exception& ex)
            {
                std::cerr << "There was exception for replierShouldNotBreakIfRequesterFails 2: " << ex.what()
                          << std::endl;
                // the first one may or may not throw, but the second must not throw.
                throw;
            }
            PRINT("Served second request by " << getpid());
        });

        // the fact that the first request break after the send message has no implication on the replier
        //        runInExternalProcess.runFork(
        //                [serveraddress, killch](){UnreliableRequester ur(serveraddress, killch);
        //                ur.breakAfterSendRequest("hello"); }
        //        );
        runInExternalProcess.runExec(
            { serveraddress, killch, "UnreliableRequester", "breakAfterSendRequest", "hello" });

        //        runInExternalProcess.runFork(
        //                [serveraddress, killch](){UnreliableRequester ur(serveraddress, killch);
        //                ur.sendReceive("hello"); }
        //        );

        runInExternalProcess.runExec({ serveraddress, killch, "UnreliableRequester", "sendReceive", "hello" });

        try
        {
            futureReplier.get();
        }
        catch (std::exception& ex)
        {
            ADD_FAILURE() << "We do not expect futureReplier.get to throw, but it threw: " << ex.what();
        }
    }

    TEST_F(ReqRepReliabilityTests, requesterShouldBeAbleToSendMessageAgainIfTheReplierIsRestored) // NOLINT
    {
        auto zmq_context = createContext();
        std::string serveraddress = m_testContext.serverAddress();
        std::string killch = m_testContext.killChannel();
        Tests::ReentrantExecutionSynchronizer synchronizer;

        auto futureRequester = std::async(std::launch::async, [serveraddress, &synchronizer, zmq_context]() {
            Requester requester(serveraddress, zmq_context);
            EXPECT_EQ(requester.sendReceive("a"), "a");
            synchronizer.notify(1);

            EXPECT_THROW(requester.sendReceive("b"), Common::ZeroMQWrapper::IIPCTimeoutException); // NOLINT

            synchronizer.notify(2);
            synchronizer.waitfor(3, 1000);
            try
            {
                EXPECT_EQ(requester.sendReceive("c"), "c");
            }
            catch (std::exception& ex)
            {
                ADD_FAILURE() << "Throw in the second send: " << ex.what();
                throw;
            }
            synchronizer.notify(4);
        });

        // simulate a replier that 'crashes' after first established req-reply, but them comes back and can serve the
        // requester.
        auto futureReplier = std::async(std::launch::async, [serveraddress, &synchronizer, zmq_context]() {
            std::unique_ptr<Replier> replier(new Replier(serveraddress, zmq_context));
            replier->serveRequest();
            synchronizer.waitfor(
                1, 3000); // necessary to ensure that the socket is not closed before sending the message.
            replier.reset();
            synchronizer.waitfor(2, 3000); // necessary to ensure that the requester timed out.
            replier.reset(new Replier(serveraddress, zmq_context));
            synchronizer.notify(3);
            replier->serveRequest();
            synchronizer.waitfor(4, 1000);
        });

        try
        {
            futureRequester.get();
        }
        catch (std::exception& ex)
        {
            ADD_FAILURE() << "Does not expect futureRequester.get to throw, but it throw: " << ex.what();
        }
        try
        {
            futureReplier.get();
        }
        catch (std::exception& ex)
        {
            ADD_FAILURE() << "Does not expect futureReplier.get to throw, but it throw: " << ex.what();
        }
    }

    TEST_F(ReqRepReliabilityTests, requesterReceivingOutOfDateReplyShouldNotDisturbCurrentRequest) // NOLINT
    {
        auto zmq_context = createContext();
        std::string serveraddress = m_testContext.serverAddress();
        std::string killch = m_testContext.killChannel();
        Tests::ReentrantExecutionSynchronizer synchronizer;
        auto futureRequester = std::async(std::launch::async, [serveraddress, &synchronizer, zmq_context]() {
            Requester requester(serveraddress, zmq_context);
            EXPECT_EQ(requester.sendReceive("a"), "a");
            EXPECT_THROW(requester.sendReceive("b"), Common::ZeroMQWrapper::IIPCTimeoutException); // NOLINT
            synchronizer.notify();
            EXPECT_EQ(requester.sendReceive("c"), "c");
            synchronizer.waitfor(2, 1000);
        });

        // simulate a replier that answers after the requester timeout.
        auto futureReplier = std::async(std::launch::async, [serveraddress, &synchronizer, zmq_context]() {
            Common::ZeroMQWrapper::ISocketReplierPtr m_replier = zmq_context->getReplier();
            m_replier->setTimeout(5000);
            m_replier->setConnectionTimeout(5000);
            m_replier->listen(serveraddress);

            data_t req = m_replier->read();
            m_replier->write(req);

            req = m_replier->read();
            synchronizer.waitfor(1, 3000); // simulate an answer after the timeout.
            m_replier->write(req);

            // restore normal answer
            req = m_replier->read();
            m_replier->write(req);
            synchronizer.notify();
        });

        try
        {
            futureRequester.get();
        }
        catch (std::exception& ex)
        {
            ADD_FAILURE() << "Does not expect futureRequester.get to throw, but it throw: " << ex.what();
        }

        try
        {
            futureReplier.get();
        }
        catch (std::exception& ex)
        {
            ADD_FAILURE() << "Does not expect futureReplier.get to throw, but it throw: " << ex.what();
        }
    }
} // namespace
#endif
