/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

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
#include <tests/Common/TestHelpers/TempDir.h>
#include <tests/Common/TestHelpers/TestExecutionSynchronizer.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>
#include <future>
#include <zmq.h>


#ifndef ARTISANBUILD

namespace
{
    using data_t = Common::ZeroMQWrapper::IReadable::data_t;
    using ::testing::NiceMock;
    using ::testing::StrictMock;

    /** uses the requester as implemented in PluginProxy::getReply and  Common::PluginApiImpl::BaseServiceAPI::getReply**/
    class Requester
    {
        Common::ZeroMQWrapper::IContextSharedPtr m_context;
        Common::ZeroMQWrapper::ISocketRequesterPtr m_requester;
    public:
        explicit Requester(const std::string& serverAddress)
        {

            m_context = Common::ZeroMQWrapper::createContext();
            m_requester = m_context->getRequester();
            m_requester->setTimeout(1000);
            m_requester->setConnectionTimeout(1000);
            m_requester->connect(serverAddress);
        }

        std::string sendReceive(const std::string& value)
        {
            m_requester->write(data_t{value});
            data_t answer = m_requester->read();
            return answer.at(0);
        }
        ~Requester() = default;
    };


    class Replier
    {
        Common::ZeroMQWrapper::IContextSharedPtr m_context;
        Common::ZeroMQWrapper::ISocketReplierPtr m_replier;
    public:
        explicit Replier(const std::string& serverAddress, int timeout = 1000)
        {

            m_context = Common::ZeroMQWrapper::createContext();
            m_replier = m_context->getReplier();
            m_replier->setTimeout(timeout);
            m_replier->setConnectionTimeout(timeout);
            m_replier->listen(serverAddress);
        }

        void serveRequest()
        {
            auto request = m_replier->read();
            m_replier->write(request);
        }
        ~Replier() = default;
    };

    class Unreliable
    {
    protected:
        Common::ZeroMQWrapper::IContextSharedPtr m_context;
        Common::ZeroMQWrapper::ISocketRequesterPtr m_requestKillChannel;

        void requestKill()
        {
            m_requestKillChannel->write(data_t{"killme"});
        }
        void notifyShutdonw()
        {
            m_requestKillChannel->write(data_t{"close"});
        }
    public:
        explicit Unreliable( const std::string & killChannelAddress )
        {
            m_context = Common::ZeroMQWrapper::createContext();
            m_requestKillChannel = m_context->getRequester();
            m_requestKillChannel->connect(killChannelAddress);
        }
    };

    class UnreliableReplier : public Unreliable
    {
        Common::ZeroMQWrapper::ISocketReplierPtr m_replier;
    public:
        UnreliableReplier(const std::string& serverAddress, const std::string& killChannelAddress)
                : Unreliable(killChannelAddress)
        {
            m_replier = m_context->getReplier();
            m_replier->setTimeout(1000);
            m_replier->setConnectionTimeout(1000);
            m_replier->listen(serverAddress);
        }

        void serveRequest()
        {
            data_t request = m_replier->read();
            m_replier->write(data_t{"granted"});
            notifyShutdonw();
        }
        void breakAfterReceiveMessage()
        {
            data_t request = m_replier->read();
            requestKill();
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }

        void servePartialReply()
        {
            auto * socketHolder = dynamic_cast<Common::ZeroMQWrapperImpl::SocketImpl*> (m_replier.get());
            assert(socketHolder);
            data_t request = m_replier->read();

            void* socket = socketHolder->skt();
            int rc;
            std::string firstElement = "firstElement";
            // send the first element but do not send the rest
            rc = zmq_send(socket, firstElement.data(), firstElement.size(), ZMQ_SNDMORE);
            assert(rc >= 0);
            requestKill();
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    };

    class UnreliableRequester : public Unreliable
    {
        Common::ZeroMQWrapper::ISocketRequesterPtr m_requester;
    public:
        UnreliableRequester( const std::string & serverAddress, const std::string & killChannelAddress )
                : Unreliable( killChannelAddress)
        {
            m_requester = m_context->getRequester();
            m_requester->setTimeout(1000);
            m_requester->setConnectionTimeout(1000);
            m_requester->connect(serverAddress);
        }

        std::string sendReceive(const std::string& value)
        {
            m_requester->write(data_t{value});
            data_t answer = m_requester->read();
            notifyShutdonw();
            return answer.at(0);
        }

        std::string breakAfterSendRequest(const std::string& value)
        {
            m_requester->write(data_t{value});
            requestKill();
            std::this_thread::sleep_for(std::chrono::seconds(2));
            return "";
        }


    };

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
                auto context = Common::ZeroMQWrapper::createContext();
                auto replierKillChannel = context->getReplier();
                replierKillChannel->setTimeout(1000);
                replierKillChannel->listen(m_killchannel);
                data_t request = replierKillChannel->read();
                if( request.at(0) == "killme")
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    ::kill(pid, SIGTERM);
                }
                else
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
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


    };


    class ReqRepReliabilityTests
            : public ::testing::Test
    {
    public:
        TestContext m_testContext;
        ReqRepReliabilityTests() :
        m_testContext()
        {

        }

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
        runInExternalProcess.runFork(
                [serveraddress, killch](){UnreliableReplier ur(serveraddress, killch); ur.serveRequest(); }
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
        runInExternalProcess.runFork(
                [serveraddress, killch](){UnreliableRequester ur(serveraddress, killch); ur.sendReceive("hello"); }
                );
        EXPECT_NO_THROW(futureReplier.get());
    }



    TEST_F( ReqRepReliabilityTests, requesterShouldRecoverAReplierFailure ) // NOLINT
    {
        std::string serveraddress = m_testContext.serverAddress();
        std::string killch = m_testContext.killChannel();

        auto futureRequester = std::async(std::launch::async, [serveraddress]() {
            Requester requester( serveraddress );
            EXPECT_THROW(requester.sendReceive("hello1"), Common::ZeroMQWrapper::IIPCTimeoutException);
            return requester.sendReceive("hello2");
        });

        RunInExternalProcess runInExternalProcess(m_testContext);
        runInExternalProcess.runFork(
                [serveraddress, killch](){UnreliableReplier ur(serveraddress, killch); ur.breakAfterReceiveMessage(); }
                );
        runInExternalProcess.runFork(
                [serveraddress, killch](){UnreliableReplier ur(serveraddress, killch); ur.serveRequest(); }
        );

        EXPECT_EQ(std::string("granted"), futureRequester.get());
    }


    TEST_F( ReqRepReliabilityTests, requesterShouldRecoverAReplierSendingBrokenMessage ) // NOLINT
    {

        std::string serveraddress = m_testContext.serverAddress();
        std::string killch = m_testContext.killChannel();
        auto futureRequester = std::async(std::launch::async, [serveraddress]() {
            Requester requester( serveraddress );
            EXPECT_THROW(requester.sendReceive("hello1"), Common::ZeroMQWrapper::IIPCTimeoutException);
            return requester.sendReceive("hello2");
        });

        RunInExternalProcess runInExternalProcess(m_testContext);
        runInExternalProcess.runFork(
                [serveraddress, killch](){UnreliableReplier ur(serveraddress, killch); ur.servePartialReply(); }

        );

        runInExternalProcess.runFork(
                [serveraddress, killch](){UnreliableReplier ur(serveraddress, killch); ur.serveRequest(); }
        );

        EXPECT_EQ(std::string("granted"), futureRequester.get());
}



    TEST_F( ReqRepReliabilityTests, replierShouldNotBreakIfRequesterFails ) // NOLINT
    {
        std::string serveraddress = m_testContext.serverAddress();
        std::string killch = m_testContext.killChannel();

        auto futureReplier = std::async(std::launch::async, [serveraddress]() {
            Replier replier( serveraddress );
            replier.serveRequest();
            replier.serveRequest();
        });

        RunInExternalProcess runInExternalProcess(m_testContext);
        // the fact that the first request break after the send message has no implication on the replier
        runInExternalProcess.runFork(
                [serveraddress, killch](){UnreliableRequester ur(serveraddress, killch); ur.breakAfterSendRequest("hello"); }

        );

        runInExternalProcess.runFork(
                [serveraddress, killch](){UnreliableRequester ur(serveraddress, killch); ur.sendReceive("hello"); }
        );
        try
        {
            futureReplier.get();
        }catch ( std::exception & ex)
        {
            EXPECT_TRUE(false) << "Does not expect futureReplier.get to throw, but it throw: "<< ex.what();
        }


    }


    TEST_F( ReqRepReliabilityTests, requesterShouldBeAbleToSendMessageAgainIfTheReplierIsRestored ) // NOLINT
    {
        std::string serveraddress = m_testContext.serverAddress();
        std::string killch = m_testContext.killChannel();
        Tests::TestExecutionSynchronizer synchronizer;
        auto futureRequester = std::async(std::launch::async, [serveraddress, &synchronizer]() {
            Requester requester( serveraddress );
            EXPECT_EQ(requester.sendReceive("a"), "a");
            EXPECT_THROW(requester.sendReceive("b"), Common::ZeroMQWrapper::IIPCTimeoutException);
            synchronizer.notify();
            EXPECT_EQ(requester.sendReceive("c"), "c");
        });

        // simulate a replier that 'crashes' after first established req-reply, but them comes back and can serve the requester.
        auto futureReplier = std::async(std::launch::async, [serveraddress, &synchronizer]() {
            std::unique_ptr<Replier> replier( new Replier(serveraddress) );
            replier->serveRequest();
            replier.reset();
            synchronizer.waitfor(3000);
            replier.reset(new Replier(serveraddress));
            replier->serveRequest();
        });

        EXPECT_NO_THROW(futureRequester.get());
        EXPECT_NO_THROW(futureReplier.get());

    }


    TEST_F( ReqRepReliabilityTests, requesterReceivingOutOfDateReplyShouldNotDisturbCurrentRequest ) // NOLINT
    {

        std::string serveraddress = m_testContext.serverAddress();
        std::string killch = m_testContext.killChannel();
        Tests::TestExecutionSynchronizer synchronizer;
        auto futureRequester = std::async(std::launch::async, [serveraddress, &synchronizer]() {
            Requester requester( serveraddress );
            EXPECT_EQ(requester.sendReceive("a"), "a");
            EXPECT_THROW(requester.sendReceive("b"), Common::ZeroMQWrapper::IIPCTimeoutException); //NOLINT
            synchronizer.notify();
            EXPECT_EQ(requester.sendReceive("c"), "c");
        });

        // simulate a replier that answers after the requester timeout.
        auto futureReplier = std::async(std::launch::async, [serveraddress, &synchronizer]() {
            auto m_context = Common::ZeroMQWrapper::createContext();
            Common::ZeroMQWrapper::ISocketReplierPtr m_replier = m_context->getReplier();
            m_replier->setTimeout(5000);
            m_replier->setConnectionTimeout(5000);
            m_replier->listen(serveraddress);

            data_t req = m_replier->read();
            m_replier->write(req);

            req = m_replier->read();
            synchronizer.waitfor(3000); // simulate an answer after the timeout.
            m_replier->write(req);

            // restore normal answer
            req = m_replier->read();
            m_replier->write(req);

        });

        EXPECT_NO_THROW(futureRequester.get()); //NOLINT
        EXPECT_NO_THROW(futureReplier.get()); //NOLINT


    }
}
#endif