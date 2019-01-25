/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef ARTISANBUILD

#include "ReqRepTestImplementations.h"

#include <Common/ZeroMQWrapper/IReadable.h>
#include <Common/ZeroMQWrapper/IContext.h>
#include <Common/ZeroMQWrapper/ISocketRequesterPtr.h>
#include <Common/ZeroMQWrapper/ISocketReplierPtr.h>
#include <Common/ZeroMQWrapper/ISocketPublisher.h>
#include <Common/ZeroMQWrapper/ISocketSubscriber.h>
#include <Common/ZeroMQWrapperImpl/SocketSubscriberImpl.h>
#include <Common/ZeroMQWrapperImpl/SocketRequesterImpl.h>
#include <Common/ZeroMQWrapperImpl/SocketReplierImpl.h>
#include <Common/ZeroMQWrapper/IIPCTimeoutException.h>
#include <tests/Common/Helpers/TempDir.h>
#include <tests/Common/Helpers/TestExecutionSynchronizer.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>
#include <future>
#include <zmq.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern char **environ;

namespace
{
    using ::testing::NiceMock;
    using ::testing::StrictMock;
    using namespace ReqRepTest;

    class TestContext
    {
        Tests::TempDir m_tempDir;
    public:
        TestContext()
        : m_tempDir( "/tmp", "reliab")
        {

        }
        std::string killChannel() const
        {
            return "ipc://" + m_tempDir.absPath("killchannel.ipc");
        }
        std::string serverAddress() const
        {
            return "ipc://" + m_tempDir.absPath("server.ipc");
        }
    };

    // in order to get a better simulation, run the requester in another process.
    class RunInExternalProcess
    {
        std::string m_killchannel;
        void monitorChild( int pid)
        {
            try{
                auto context = createContext();
                auto replierKillChannel = context->getReplier();
                replierKillChannel->setTimeout(1000);
                replierKillChannel->listen(m_killchannel);
                data_t request = replierKillChannel->read();
                if( request.at(0) == "killme")
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    ::kill(pid, SIGTERM);
                }
                else
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }catch (std::exception & ex)
            {
                std::cout << "monitor child: " << ex.what() << std::endl;

            }

        }

        void runChild(std::function<void()> && functor)
        {
            try
            {
                functor();

            }catch ( std::exception & ex)
            {
                std::cerr << "Runchild: " << ex.what() << std::endl;
            }
            ::exit(0);
        }


    public:
        explicit RunInExternalProcess( const TestContext & context):
            m_killchannel( context.killChannel())
        {

        }

        void runFork(std::function<void()>  functor)
        {
            std::cerr << "Fork from "<<::getpid() << std::endl;
            pid_t child = fork();

            if ( child == -1)
            {
                throw std::logic_error( "Failed to create process");
            }
            if ( child == 0)
            {
                runChild(std::move(functor));
            }
            else
            {
                monitorChild(child);
            }
        }

        void runExec(std::vector<std::string> args)
        {
            // Find executable
            std::string exe = CMAKE_CURRENT_BINARY_DIR;
            exe += "/TestReqRepTool";
            // Create argument list
            char** newargv = static_cast<char**>(malloc(sizeof(char*)*(args.size()+2)));
            const char* exe_cstr = exe.c_str();
            newargv[0] = const_cast<char*>(exe_cstr); // Not actually modified
            for (size_t i=0; i< args.size();i++)
            {
                newargv[i+1] = const_cast<char*>(args[i].c_str()); // Not actually modified
            }
            newargv[args.size() + 1] = nullptr;


            char *newenviron[] = { nullptr };


            // Fork

            std::cerr << "Fork from "<<::getpid() << std::endl;
            std::cerr << "Will exec "<< exe << std::endl;

            pid_t child = fork();

            if ( child == -1)
            {
                throw std::logic_error( "Failed to create process");
            }
            if ( child != 0)
            {
                monitorChild(child);
                return;
            }

            // Exec
            ::execve(exe_cstr, newargv, newenviron);
            perror("execve");   /* execve() returns only on error */
            _exit(70);
        }


    };


    class ReqRepReliabilityTests
            : public ::testing::Test
    {
    public:
        TestContext m_testContext;
        ReqRepReliabilityTests() = default;
    };

    TEST_F( ReqRepReliabilityTests, normalReqReplyShouldWork ) // NOLINT
    {
        RunInExternalProcess runInExternalProcess(m_testContext);
        std::string serveraddress = m_testContext.serverAddress();
        std::string killch = m_testContext.killChannel();

        auto futureRequester = std::async(std::launch::async, [serveraddress]() {
            Requester requester( serveraddress );
            return requester.sendReceive("hello");
        });
//        runInExternalProcess.runFork(
//                [serveraddress, killch](){UnreliableReplier ur(serveraddress, killch); ur.serveRequest(); }
//                );
        runInExternalProcess.runExec(
                {serveraddress, killch,"UnreliableReplier","serveRequest" }
                );

        EXPECT_EQ(std::string("granted"), futureRequester.get());
    }


    TEST_F( ReqRepReliabilityTests, normalReqReplyShouldWorkUsingReply ) // NOLINT
    {
        RunInExternalProcess runInExternalProcess(m_testContext);
        std::string serveraddress = m_testContext.serverAddress();
        std::string killch = m_testContext.killChannel();

        auto futureReplier = std::async(std::launch::async, [serveraddress]() {
            Replier replier( serveraddress );
            replier.serveRequest();
        });
//        runInExternalProcess.runFork(
//                [serveraddress, killch](){UnreliableRequester ur(serveraddress, killch); ur.sendReceive("hello"); }
//                );
        runInExternalProcess.runExec(
                {serveraddress, killch,"UnreliableRequester","sendReceive","hello"}
                );
        EXPECT_NO_THROW(futureReplier.get()); //NOLINT
    }


    TEST_F( ReqRepReliabilityTests, requesterShouldRecoverAReplierFailure ) // NOLINT
    {
        RunInExternalProcess runInExternalProcess(m_testContext);
        std::string serveraddress = m_testContext.serverAddress();
        std::string killch = m_testContext.killChannel();

        auto futureRequester = std::async(std::launch::async, [serveraddress]() {
            Requester requester( serveraddress );
            EXPECT_THROW(requester.sendReceive("hello1"), Common::ZeroMQWrapper::IIPCTimeoutException); //NOLINT
            return requester.sendReceive("hello2");
        });

//        runInExternalProcess.runFork(
//                [serveraddress, killch](){UnreliableReplier ur(serveraddress, killch); ur.breakAfterReceiveMessage(); }
//                );
        runInExternalProcess.runExec(
                {serveraddress, killch,"UnreliableReplier","breakAfterReceiveMessage" }
                );
//        runInExternalProcess.runFork(
//                [serveraddress, killch](){UnreliableReplier ur(serveraddress, killch); ur.serveRequest(); }
//        );
        runInExternalProcess.runExec(
                {serveraddress, killch,"UnreliableReplier","serveRequest" }
        );

        EXPECT_EQ(std::string("granted"), futureRequester.get());
    }


    TEST_F( ReqRepReliabilityTests, requesterShouldRecoverAReplierSendingBrokenMessage ) // NOLINT
    {
        RunInExternalProcess runInExternalProcess(m_testContext);
        std::string serveraddress = m_testContext.serverAddress();
        std::string killch = m_testContext.killChannel();

        auto futureRequester = std::async(std::launch::async, [serveraddress]() {
            Requester requester( serveraddress );
            EXPECT_THROW(requester.sendReceive("hello1"), Common::ZeroMQWrapper::IIPCTimeoutException); //NOLINT
            return requester.sendReceive("hello2");
        });

//        runInExternalProcess.runFork(
//                [serveraddress, killch](){UnreliableReplier ur(serveraddress, killch); ur.servePartialReply(); }
//
//        );

        runInExternalProcess.runExec(
                {serveraddress, killch,"UnreliableReplier","servePartialReply" }
        );

//        runInExternalProcess.runFork(
//                [serveraddress, killch](){UnreliableReplier ur(serveraddress, killch); ur.serveRequest(); }
//        );

        runInExternalProcess.runExec(
                {serveraddress, killch,"UnreliableReplier","serveRequest" }
        );

        EXPECT_EQ(std::string("granted"), futureRequester.get());
}


    TEST_F( ReqRepReliabilityTests, replierShouldNotBreakIfRequesterFails ) // NOLINT
    {
        RunInExternalProcess runInExternalProcess(m_testContext);
        std::string serveraddress = m_testContext.serverAddress();
        std::string killch = m_testContext.killChannel();

        auto futureReplier = std::async(std::launch::async, [serveraddress]() {
            Replier replier( serveraddress, 10000 );
            try
            {
                replier.serveRequest();
            }
            catch ( std::exception & ex)
            {
                std::cerr << "There was exception for replierShouldNotBreakIfRequesterFails 1: " << ex.what() << std::endl;
                // the first one may or may not throw, but the second must not throw.
            }

            try
            {
                replier.serveRequest();
            }
            catch ( std::exception & ex)
            {
                std::cerr << "There was exception for replierShouldNotBreakIfRequesterFails 2: " << ex.what() << std::endl;
                // the first one may or may not throw, but the second must not throw.
                throw;
            }
        });

        // the fact that the first request break after the send message has no implication on the replier
//        runInExternalProcess.runFork(
//                [serveraddress, killch](){UnreliableRequester ur(serveraddress, killch); ur.breakAfterSendRequest("hello"); }
//        );
        runInExternalProcess.runExec(
                {serveraddress, killch,"UnreliableRequester","breakAfterSendRequest","hello" }
        );

//        runInExternalProcess.runFork(
//                [serveraddress, killch](){UnreliableRequester ur(serveraddress, killch); ur.sendReceive("hello"); }
//        );

        runInExternalProcess.runExec(
                {serveraddress, killch,"UnreliableRequester","sendReceive","hello" }
        );

        try
        {
            futureReplier.get();
        }
        catch ( std::exception & ex)
        {
            ADD_FAILURE() << "We do not expect futureReplier.get to throw, but it threw: "<< ex.what();
        }


    }


    TEST_F( ReqRepReliabilityTests, requesterShouldBeAbleToSendMessageAgainIfTheReplierIsRestored ) // NOLINT
    {
        std::string serveraddress = m_testContext.serverAddress();
        std::string killch = m_testContext.killChannel();
        Tests::ReentrantExecutionSynchronizer synchronizer;

        auto futureRequester = std::async(std::launch::async, [serveraddress, &synchronizer]() {
            Requester requester( serveraddress );
            EXPECT_EQ(requester.sendReceive("a"), "a");
            synchronizer.notify(1);

            EXPECT_THROW(requester.sendReceive("b"), Common::ZeroMQWrapper::IIPCTimeoutException); //NOLINT

            synchronizer.notify(2);
            synchronizer.waitfor(3, 1000);
            try
            {
                EXPECT_EQ(requester.sendReceive("c"), "c");
            }catch (std::exception & ex)
            {
                ADD_FAILURE() << "Throw in the second send: "<< ex.what();
                throw;
            }
            synchronizer.notify(4);
        });

        // simulate a replier that 'crashes' after first established req-reply, but them comes back and can serve the requester.
        auto futureReplier = std::async(std::launch::async, [serveraddress, &synchronizer]() {
            std::unique_ptr<Replier> replier( new Replier(serveraddress) );
            replier->serveRequest();
            synchronizer.waitfor(1, 3000); // necessary to ensure that the socket is not closed before sending the message.
            replier.reset();
            synchronizer.waitfor(2, 3000); // necessary to ensure that the requester timed out.
            replier.reset(new Replier(serveraddress));
            synchronizer.notify(3);
            replier->serveRequest();
            synchronizer.waitfor(4, 1000);
        });

        try
        {
            futureRequester.get();
        }catch ( std::exception & ex)
        {
            ADD_FAILURE() << "Does not expect futureRequester.get to throw, but it throw: "<< ex.what();
        }
        try
        {
            futureReplier.get();
        }catch ( std::exception & ex)
        {
            ADD_FAILURE() << "Does not expect futureReplier.get to throw, but it throw: "<< ex.what();
        }


    }


    TEST_F( ReqRepReliabilityTests, requesterReceivingOutOfDateReplyShouldNotDisturbCurrentRequest ) // NOLINT
    {

        std::string serveraddress = m_testContext.serverAddress();
        std::string killch = m_testContext.killChannel();
        Tests::ReentrantExecutionSynchronizer synchronizer;
        auto futureRequester = std::async(std::launch::async, [serveraddress, &synchronizer]() {
            Requester requester( serveraddress );
            EXPECT_EQ(requester.sendReceive("a"), "a");
            EXPECT_THROW(requester.sendReceive("b"), Common::ZeroMQWrapper::IIPCTimeoutException); //NOLINT
            synchronizer.notify();
            EXPECT_EQ(requester.sendReceive("c"), "c");
            synchronizer.waitfor(2, 1000);
        });

        // simulate a replier that answers after the requester timeout.
        auto futureReplier = std::async(std::launch::async, [serveraddress, &synchronizer]() {
            auto context = createContext();
            Common::ZeroMQWrapper::ISocketReplierPtr m_replier = context->getReplier();
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
        }catch ( std::exception & ex)
        {
            ADD_FAILURE() << "Does not expect futureRequester.get to throw, but it throw: "<< ex.what();
        }

        try
        {
            futureReplier.get();
        }catch ( std::exception & ex)
        {
            ADD_FAILURE() << "Does not expect futureReplier.get to throw, but it throw: "<< ex.what();
        }


    }
}
#endif