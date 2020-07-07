/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <modules/CommsComponent/AsyncMessager.h>
#include <boost/thread/thread.hpp>
#include <tests/Common/Helpers/TestExecutionSynchronizer.h>
using namespace Comms; 

struct MessagesAppender
{
    std::vector<std::string> & m_messages;
    Tests::TestExecutionSynchronizer & m_synchronizer;
    MessagesAppender(std::vector<std::string> & messages,
            Tests::TestExecutionSynchronizer & synchronizer):
            m_messages(messages), m_synchronizer(synchronizer){}
    void operator()(std::string newMessage){
        m_messages.emplace_back(std::move(newMessage));
        m_synchronizer.notify();
    }
};

TEST(TestAsyncMessager, MessagesCanBeInterchangedByAsyncMessager) // NOLINT
{
    using ListStrings = std::vector<std::string>;
    boost::asio::io_service m_io;

    ListStrings firstReceivedMessages1;
    ListStrings firstReceivedMessages2;
    Tests::TestExecutionSynchronizer synchronizer(3);

    auto [m1,m2] = CommsContext::setupPairOfConnectedSockets(m_io, MessagesAppender{firstReceivedMessages1, synchronizer} , MessagesAppender{firstReceivedMessages2, synchronizer} ); 

    std::thread thread = CommsContext::startThread(m_io); 

    std::string message{"basictest"};
    std::string message2{"basictest2"};
    std::string fromm1{"from_m1"};
    m1->sendMessage(fromm1);
    m2->sendMessage(message);
    m2->sendMessage(message2);
    EXPECT_TRUE(synchronizer.waitfor(1000));
    m1->push_stop();
    m2->push_stop();
    thread.join();
    ASSERT_EQ(firstReceivedMessages1.size(), 2);
    ASSERT_EQ(firstReceivedMessages2.size(), 1);
    EXPECT_EQ(message, firstReceivedMessages1.at(0));
    EXPECT_EQ(message2, firstReceivedMessages1.at(1));
    EXPECT_EQ(fromm1, firstReceivedMessages2.at(0));
}


TEST(TestAsyncMessager, messagesWithDifferentSizesCanBeSentAndReceived) // NOLINT
{
    using ListStrings = std::vector<std::string>;
    boost::asio::io_service m_io;

    ListStrings firstReceivedMessages1;
    ListStrings firstReceivedMessages2;

    // sending up to almost 10MB in a single message 
    std::vector<int> vecsizes{10,100,1000, 1'0000, 100'000, 1'000'000, 10'000'000}; 
    Tests::TestExecutionSynchronizer synchronizer(vecsizes.size());

    auto [m1,m2] = CommsContext::setupPairOfConnectedSockets(m_io, MessagesAppender{firstReceivedMessages1, synchronizer} , MessagesAppender{firstReceivedMessages2, synchronizer} ); 

    std::thread thread = CommsContext::startThread(m_io); 

    for( int messageSize : vecsizes)
    {

        std::string message(messageSize, 'a'); 
        m1->sendMessage(message);         
    }

    EXPECT_TRUE(synchronizer.waitfor(5000));
    m2->push_stop(); 
    m1->push_stop(); 
    thread.join(); 

    ASSERT_EQ(firstReceivedMessages2.size(), vecsizes.size());
    for(size_t i=0; i < vecsizes.size(); i++)
    {
        EXPECT_EQ(firstReceivedMessages2.at(i).size(), vecsizes.at(i)); 
    }    
}


TEST(TestAsyncMessager, reconstructedMessageShouldBeTheSameASOriginal) // NOLINT
{
    using ListStrings = std::vector<std::string>;
    boost::asio::io_service m_io;

    ListStrings firstReceivedMessages1;
    ListStrings firstReceivedMessages2;

    std::string message = std::string(AsyncMessager::capacity,'a') + std::string(AsyncMessager::capacity,'b'); 
    EXPECT_EQ(message.size(), AsyncMessager::capacity*2); 

    Tests::TestExecutionSynchronizer synchronizer(1);

    auto [m1,m2] = CommsContext::setupPairOfConnectedSockets(m_io, MessagesAppender{firstReceivedMessages1, synchronizer} , MessagesAppender{firstReceivedMessages2, synchronizer} ); 

    std::thread thread = CommsContext::startThread(m_io); 

    m1->sendMessage(message); 
    EXPECT_TRUE(synchronizer.waitfor(5000));
    m2->push_stop(); 
    m1->push_stop(); 
    thread.join(); 

    ASSERT_EQ(firstReceivedMessages2.size(), 1);
    ASSERT_EQ(firstReceivedMessages2.at(0), message);    
}


TEST(TestAsyncMessager, shouldSupportBinaryMessagesAsWell) // NOLINT
{
    using ListStrings = std::vector<std::string>;
    boost::asio::io_service m_io;

    ListStrings firstReceivedMessages1;
    ListStrings firstReceivedMessages2;

    // the full spectrum of binary values
    std::array<char,256> fullBuffer; 
    char i=0; 
    for(char& v: fullBuffer)
    {
        v=i++; 
    }

    std::string message = std::string{fullBuffer.begin(), fullBuffer.end()}; 
    EXPECT_EQ(message.size(), fullBuffer.size()); 

    Tests::TestExecutionSynchronizer synchronizer(1);

    auto [m1,m2] = CommsContext::setupPairOfConnectedSockets(m_io, MessagesAppender{firstReceivedMessages1, synchronizer} , MessagesAppender{firstReceivedMessages2, synchronizer} ); 

    std::thread thread = CommsContext::startThread(m_io); 

    m1->sendMessage(message); 
    EXPECT_TRUE(synchronizer.waitfor(5000));
    m2->push_stop(); 
    m1->push_stop(); 
    thread.join(); 

    ASSERT_EQ(firstReceivedMessages2.size(), 1);
    ASSERT_EQ(firstReceivedMessages2.at(0), message);
    ASSERT_EQ(message[10], 10);     
}

