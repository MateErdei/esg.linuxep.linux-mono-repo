//
// Created by pair on 07/06/18.
//


#include <gtest/gtest.h>

#include <Common/Exceptions/Print.h>

#include <Common/ZeroMQWrapper/ISocketReplier.h>
#include <Common/ZeroMQWrapper/ISocketRequester.h>
#include <Common/ZeroMQWrapper/IIPCTimeoutException.h>

#include <Common/ZeroMQWrapperImpl/ContextImpl.h>
#include <sys/socket.h>
#include <chrono>
#include <thread>

using Common::ZeroMQWrapper::ISocketReplierPtr;

namespace
{
    TEST(SocketReplierImpl, creation) // NOLINT
    {
        std::unique_ptr<Common::ZeroMQWrapper::IContext> context = Common::ZeroMQWrapper::createContext();
        ASSERT_NE(context.get(),nullptr);
        ISocketReplierPtr socket = context->getReplier();
        EXPECT_NE(socket.get(),nullptr);
    }

    TEST(SocketReplierImpl, listen) // NOLINT
    {
        std::unique_ptr<Common::ZeroMQWrapper::IContext> context = Common::ZeroMQWrapper::createContext();
        ASSERT_NE(context.get(),nullptr);
        auto socket = context->getReplier();
        EXPECT_NE(socket.get(),nullptr);
        socket->listen("inproc://ListenTest");
    }

    TEST(SocketReplierImpl, transfer) // NOLINT
    {
        std::unique_ptr<Common::ZeroMQWrapper::IContext> context = Common::ZeroMQWrapper::createContext();
        ASSERT_NE(context.get(),nullptr);
        auto replier = context->getReplier();
        auto requester = context->getRequester();
        EXPECT_NE(replier.get(),nullptr);
        replier->listen("inproc://transferTest");
//        std::this_thread::sleep_for( std::chrono::milliseconds(100) );
        requester->connect("inproc://transferTest");

        using data_t = std::vector<std::string>;

        data_t input{"FOO","BAR"};
        while (true)
        {
            try
            {
                requester->write(input);
                break;
            }
            catch( const Common::ZeroMQWrapper::IIPCTimeoutException& e)
            {
                PRINT("Ignoring timeout sending request");
            }

            std::this_thread::sleep_for( std::chrono::milliseconds(1000) );
        }
        data_t output = replier->read();

        EXPECT_EQ(input,output);

        data_t input2{"RES"};
        replier->write(input2);

        data_t output2 = requester->read();

        EXPECT_EQ(input2,output2);
    }
}
