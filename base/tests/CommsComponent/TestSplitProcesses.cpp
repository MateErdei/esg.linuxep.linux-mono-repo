/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <boost/thread/thread.hpp>
#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <modules/CommsComponent/SplitProcesses.h>

//#include <tests/Common/Helpers/TestExecutionSynchronizer.h>
using namespace CommsComponent;

class TestSplitProcesses : public ::testing::Test
{
    public: 
    TestSplitProcesses()
    {
         testing::FLAGS_gtest_death_test_style="threadsafe";
    }
};

TEST_F(TestSplitProcesses, SimpleHelloWorldDemonstration) // NOLINT
{
    testing::FLAGS_gtest_death_test_style="threadsafe";
    auto childProcess=[](std::shared_ptr<MessageChannel> channel, OtherSideApi& parentProxy)
    {
        std::string message; 
        channel->pop(message); 
        if ( message != "hello")
        {
            throw std::runtime_error("Did not received hello"); 
        }
        parentProxy.pushMessage("world"); 
    };

    auto parentProcess=[](std::shared_ptr<MessageChannel> channel, OtherSideApi& childProxy){
        childProxy.pushMessage("hello");
        std::string message; 
        channel->pop(message); 
        if (message != "world")
        {
            throw std::runtime_error("Did not receive world"); 
        }        
    };

    int exitCode = splitProcessesReactors(parentProcess, childProcess); 
    EXPECT_EQ(exitCode, 0); 
}

TEST_F(TestSplitProcesses, ParentIsNotifiedOnChildExit) // NOLINT
{
    auto childProcess=[](std::shared_ptr<MessageChannel> /*channel*/, OtherSideApi& /*parentProxy*/)
    {
    };

    auto parentProcess=[](std::shared_ptr<MessageChannel> channel, OtherSideApi& childProxy){
        childProxy.pushMessage("hello");
        std::string message; 
        try
        {
            channel->pop(message); 
        }
        catch(ChannelClosedException&)
        {
            return; 
        }
        throw std::runtime_error("Did not receive closed channel exception");         
    };

    int exitCode = splitProcessesReactors(parentProcess, childProcess); 
    EXPECT_EQ(exitCode, 0); 
}
