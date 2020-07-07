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

//sendMessage(str)
//str + '1' or '0'
//break_str_into_chunks( 4095) (4096)
//'stop'
//
//
//recv(buffer)
//check last (0): combine previous messages
//if (1) append to a pending_parts

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

    std::vector<int> vecsizes{10,100,1000}; //TODO add to bigger values
    Tests::TestExecutionSynchronizer synchronizer(vecsizes.size());

    auto [m1,m2] = CommsContext::setupPairOfConnectedSockets(m_io, MessagesAppender{firstReceivedMessages1, synchronizer} , MessagesAppender{firstReceivedMessages2, synchronizer} ); 

    std::thread thread = CommsContext::startThread(m_io); 

    for( int messageSize : vecsizes)
    {

        std::string message(messageSize, 'a'); 
        m1->sendMessage(message);         
    }

    EXPECT_TRUE(synchronizer.waitfor(1000));
    m2->push_stop(); 
    m1->push_stop(); 
    thread.join(); 

    ASSERT_EQ(firstReceivedMessages2.size(), vecsizes.size());
    for(size_t i=0; i < vecsizes.size(); i++)
    {
        EXPECT_EQ(firstReceivedMessages2.at(i).size(), vecsizes.at(i)); 
    }    
}

