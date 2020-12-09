/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/Exceptions/Print.h>
#include <Common/ZMQWrapperApiImpl/ContextImpl.h>
#include <Common/ZeroMQWrapper/IIPCTimeoutException.h>
#include <Common/ZeroMQWrapper/ISocketReplier.h>
#include <Common/ZeroMQWrapper/ISocketRequester.h>
#include <gtest/gtest.h>
#include <sys/socket.h>

#include <chrono>
#include <thread>

using Common::ZeroMQWrapper::ISocketReplierPtr;

namespace
{
    TEST(SocketReplierImpl, creation) // NOLINT
    {
        auto context = Common::ZMQWrapperApi::createContext();
        ASSERT_NE(context.get(), nullptr);
        ISocketReplierPtr socket = context->getReplier();
        EXPECT_NE(socket.get(), nullptr);
    }

    // cppcheck-suppress syntaxError
    TEST(SocketReplierImpl, listen) // NOLINT
    {
        auto context = Common::ZMQWrapperApi::createContext();
        ASSERT_NE(context.get(), nullptr);
        auto socket = context->getReplier();
        EXPECT_NE(socket.get(), nullptr);
        socket->listen("inproc://ListenTest");
    }

    TEST(SocketReplierImpl, transfer) // NOLINT
    {
        auto context = Common::ZMQWrapperApi::createContext();
        ASSERT_NE(context.get(), nullptr);
        auto replier = context->getReplier();
        auto requester = context->getRequester();
        ASSERT_NE(replier.get(), nullptr);
        replier->listen("inproc://transferTest");
        //        std::this_thread::sleep_for( std::chrono::milliseconds(100) );
        requester->connect("inproc://transferTest");

        using data_t = std::vector<std::string>;

        data_t input{ "FOO", "BAR" };
        while (true)
        {
            try
            {
                requester->write(input);
                break;
            }
            catch (const Common::ZeroMQWrapper::IIPCTimeoutException& e)
            {
                PRINT("Ignoring timeout sending request");
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        data_t output = replier->read();

        EXPECT_EQ(input, output);

        data_t input2{ "RES" };
        replier->write(input2);

        data_t output2 = requester->read();

        EXPECT_EQ(input2, output2);
    }

    void repeatWrite(Common::ZeroMQWrapper::IWritable& requester, Common::ZeroMQWrapper::data_t& input)
    {
        while (true)
        {
            try
            {
                requester.write(input);
                break;
            }
            catch (const Common::ZeroMQWrapper::IIPCTimeoutException& e)
            {
                PRINT("Ignoring timeout sending request");
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }

    TEST(SocketReplierImpl, context_remains_after_holder_deleted) // NOLINT
    {
        auto context = Common::ZMQWrapperApi::createContext();
        auto replier = context->getReplier();
        ASSERT_NE(replier.get(), nullptr);
        auto requester = context->getRequester();
        ASSERT_NE(requester.get(), nullptr);

        // The context object is deleted here, but the ZMQ context remains - in shared_ptr owned by replier and
        // requester.
        context.reset();

        replier->listen("inproc://contextTest");
        requester->connect("inproc://contextTest");

        using data_t = Common::ZeroMQWrapper::data_t;
        data_t input{ "FOO", "BAR" };
        repeatWrite(*requester, input);
        data_t output = replier->read();

        EXPECT_EQ(input, output);

        data_t input2{ "RES" };
        replier->write(input2);

        data_t output2 = requester->read();

        EXPECT_EQ(input2, output2);

        replier.reset();
        requester.reset();
    }
} // namespace
