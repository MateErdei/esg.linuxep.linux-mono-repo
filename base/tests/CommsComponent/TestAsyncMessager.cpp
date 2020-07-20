/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <boost/thread/thread.hpp>
#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <modules/CommsComponent/AsyncMessager.h>
#include <tests/Common/Helpers/TestExecutionSynchronizer.h>
#include <tests/Common/Helpers/LogInitializedTests.h>

using namespace CommsComponent;

struct MessagesAppender
{
    static constexpr const char *Command2Throw = "Command2Throw";
    std::vector<std::string> &m_messages;
    Tests::TestExecutionSynchronizer &m_synchronizer;

    MessagesAppender(std::vector<std::string> &messages, Tests::TestExecutionSynchronizer &synchronizer) :
            m_messages(messages),
            m_synchronizer(synchronizer)
    {
    }
    void operator()(std::string newMessage)
    {
        if (newMessage == Command2Throw)
        {
            throw std::runtime_error("unacceptable message");
        }
        m_messages.emplace_back(std::move(newMessage));
        m_synchronizer.notify();
    }
};
class TestAsyncMessager : public LogOffInitializedTests{};

TEST_F(TestAsyncMessager, MessagesCanBeInterchangedByAsyncMessager) // NOLINT
{
    using ListStrings = std::vector<std::string>;
    boost::asio::io_service m_io;

    ListStrings receivedMessages1;
    ListStrings receivedMessages2;
    Tests::TestExecutionSynchronizer synchronizer(3);

    auto [m1, m2] = CommsContext::setupPairOfConnectedSockets(
        m_io,
        MessagesAppender{ receivedMessages1, synchronizer },
        MessagesAppender{ receivedMessages2, synchronizer });

    std::thread thread = CommsContext::startThread(m_io);

    std::string message{ "basictest" };
    std::string message2{ "basictest2" };
    std::string from1{ "from_m1" };
    m1->sendMessage(from1);
    m2->sendMessage(message);
    m2->sendMessage(message2);
    EXPECT_TRUE(synchronizer.waitfor(1000));
    m1->push_stop();
    m2->push_stop();
    thread.join();
    ASSERT_EQ(receivedMessages1.size(), 2);
    ASSERT_EQ(receivedMessages2.size(), 1);
    EXPECT_EQ(message, receivedMessages1.at(0));
    EXPECT_EQ(message2, receivedMessages1.at(1));
    EXPECT_EQ(from1, receivedMessages2.at(0));
}

TEST_F(TestAsyncMessager, Utf8MessagesArePreserved) // NOLINT
{
    using ListStrings = std::vector<std::string>;
    boost::asio::io_service m_io;

    ListStrings receivedMessages1;
    ListStrings receivedMessages2;
    Tests::TestExecutionSynchronizer synchronizer(1);

    auto [m1, m2] = CommsContext::setupPairOfConnectedSockets(
        m_io,
        MessagesAppender{ receivedMessages1, synchronizer },
        MessagesAppender{ receivedMessages2, synchronizer });

    std::thread thread = CommsContext::startThread(m_io);

    std::string message{ "中英字典" };
    m2->sendMessage(message);
    EXPECT_TRUE(synchronizer.waitfor(1000));
    m1->push_stop();
    m2->push_stop();
    thread.join();
    ASSERT_EQ(receivedMessages1.size(), 1);
    EXPECT_EQ(message, receivedMessages1.at(0));
}


TEST_F(TestAsyncMessager, messagesWithDifferentSizesCanBeSentAndReceived) // NOLINT
{
    using ListStrings = std::vector<std::string>;
    boost::asio::io_service m_io;

    ListStrings receivedMessages1;
    ListStrings receivedMessages2;

    // sending up to almost 10MB in a single message
    std::vector<int> vecsizes{ 10, 100, 1000, 1'0000, 100'000, 1'000'000, 10'000'000 };
    Tests::TestExecutionSynchronizer synchronizer(vecsizes.size());

    auto [m1, m2] = CommsContext::setupPairOfConnectedSockets(
        m_io,
        MessagesAppender{ receivedMessages1, synchronizer },
        MessagesAppender{ receivedMessages2, synchronizer });

    std::thread thread = CommsContext::startThread(m_io);

    for (int messageSize : vecsizes)
    {
        std::string message(messageSize, 'a');
        m1->sendMessage(message);
    }

    EXPECT_TRUE(synchronizer.waitfor(5000));
    m2->push_stop();
    m1->push_stop();
    thread.join();

    ASSERT_EQ(receivedMessages2.size(), vecsizes.size());
    for (size_t i = 0; i < vecsizes.size(); i++)
    {
        EXPECT_EQ(receivedMessages2.at(i).size(), vecsizes.at(i));
    }
}

TEST_F(TestAsyncMessager, reconstructedMessageShouldBeTheSameASOriginal) // NOLINT
{
    using ListStrings = std::vector<std::string>;
    boost::asio::io_service m_io;

    ListStrings receivedMessages1;
    ListStrings receivedMessages2;

    std::string message = std::string(AsyncMessager::Capacity, 'a') + std::string(AsyncMessager::Capacity, 'b');
    EXPECT_EQ(message.size(), AsyncMessager::Capacity * 2);

    Tests::TestExecutionSynchronizer synchronizer(1);

    auto [m1, m2] = CommsContext::setupPairOfConnectedSockets(
        m_io,
        MessagesAppender{ receivedMessages1, synchronizer },
        MessagesAppender{ receivedMessages2, synchronizer });

    std::thread thread = CommsContext::startThread(m_io);

    m1->sendMessage(message);
    EXPECT_TRUE(synchronizer.waitfor(5000));
    m2->push_stop();
    m1->push_stop();
    thread.join();

    ASSERT_EQ(receivedMessages2.size(), 1);
    ASSERT_EQ(receivedMessages2.at(0), message);
}

TEST_F(TestAsyncMessager, shouldSupportBinaryMessagesAsWell) // NOLINT
{
    using ListStrings = std::vector<std::string>;
    boost::asio::io_service m_io;

    ListStrings receivedMessages1;
    ListStrings receivedMessages2;

    // the full spectrum of binary values
    std::array<char, 256> fullBuffer;
    char i = 0;
    for (char& v : fullBuffer)
    {
        v = i++;
    }

    std::string message = std::string{ fullBuffer.begin(), fullBuffer.end() };
    EXPECT_EQ(message.size(), fullBuffer.size());

    Tests::TestExecutionSynchronizer synchronizer(1);

    auto [m1, m2] = CommsContext::setupPairOfConnectedSockets(
        m_io,
        MessagesAppender{ receivedMessages1, synchronizer },
        MessagesAppender{ receivedMessages2, synchronizer });

    std::thread thread = CommsContext::startThread(m_io);

    m1->sendMessage(message);
    EXPECT_TRUE(synchronizer.waitfor(5000));
    m2->push_stop();
    m1->push_stop();
    thread.join();

    ASSERT_EQ(receivedMessages2.size(), 1);
    ASSERT_EQ(receivedMessages2.at(0), message);
    ASSERT_EQ(message[10], 10);
}


TEST_F(TestAsyncMessager, anySideShouldBeAbleToStopTheCommunication) // NOLINT
{
    using ListStrings = std::vector<std::string>;
    boost::asio::io_service m_io;

    ListStrings receivedMessages1;
    ListStrings receivedMessages2;

    std::string message = "test";
    Tests::TestExecutionSynchronizer synchronizer(1);

    auto [m1, m2] = CommsContext::setupPairOfConnectedSockets(
        m_io,
        MessagesAppender{ receivedMessages1, synchronizer },
        MessagesAppender{ receivedMessages2, synchronizer });

    std::thread thread = CommsContext::startThread(m_io);

    m1->sendMessage(message);
    EXPECT_TRUE(synchronizer.waitfor(5000));
    m2->push_stop();
    // it is not necessary to send the push stop to m1 as well, as it has been 
    // notified that the other is closed.
    thread.join();

    ASSERT_EQ(receivedMessages2.size(), 1);
    ASSERT_EQ(receivedMessages2.at(0), message);
}


TEST_F(TestAsyncMessager, asyncMessagersShouldBeResistentToExceptionsTriggeredIntheCallBack) // NOLINT
{
    using ListStrings = std::vector<std::string>;
    boost::asio::io_service m_io;

    ListStrings receivedMessages1;
    ListStrings receivedMessages2;

    std::string message = "test";
    std::string willTriggerThrow = MessagesAppender::Command2Throw;
    Tests::TestExecutionSynchronizer synchronizer(1);

    auto [m1, m2] = CommsContext::setupPairOfConnectedSockets(
        m_io,
        MessagesAppender{ receivedMessages1, synchronizer },
        MessagesAppender{ receivedMessages2, synchronizer });

    std::thread thread = CommsContext::startThread(m_io);

    m1->sendMessage(willTriggerThrow); // this message will trigger throw and will not be 'delivered' 
    m1->sendMessage(message);
    EXPECT_TRUE(synchronizer.waitfor(5000));
    m2->push_stop();
    thread.join();

    // The second message was still delivered
    ASSERT_EQ(receivedMessages2.size(), 1);
    ASSERT_EQ(receivedMessages2.at(0), message);
}


TEST_F(TestAsyncMessager, emptyMessagesWillBeRejectedAndNotTransmitted) // NOLINT
{
    using ListStrings = std::vector<std::string>;
    boost::asio::io_service m_io;

    ListStrings receivedMessages1;
    ListStrings receivedMessages2;

    std::string emptyMessage;
    Tests::TestExecutionSynchronizer synchronizer(1);

    auto [m1, m2] = CommsContext::setupPairOfConnectedSockets(
        m_io,
        MessagesAppender{ receivedMessages1, synchronizer },
        MessagesAppender{ receivedMessages2, synchronizer });

    std::thread thread = CommsContext::startThread(m_io);

    EXPECT_THROW(m1->sendMessage(emptyMessage), std::runtime_error); 
    m1->push_stop();
    (void) m2; 
    thread.join();
    // no message sent
    ASSERT_EQ(receivedMessages2.size(), 0);
}

