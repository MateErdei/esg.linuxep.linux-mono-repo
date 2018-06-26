/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include "Common/ReactorImpl/GenericCallbackListener.h"
using namespace Common::Reactor;

namespace
{
    int callbackCalled = 0;
}

class TestGenericCallbackListener : public ::testing::Test
{
public:
    std::vector<std::string> m_callbackData;
    void SetUp() override
    {

    }
    void TearDown() override
    {

    }

    void callback(std::vector<std::string> data)
    {
        m_callbackData = data;
    }

};


ProcessInstruction pureCallBackFunction(std::vector<std::string> data)
{
    callbackCalled = 1;
    return ProcessInstruction::CONTINUE;
}

TEST_F(TestGenericCallbackListener, callbackAsPureFunction)
{
    std::vector<std::string> data = {"arg1","arg2"};
    callbackCalled = 0;
    GenericCallbackListener listener(pureCallBackFunction);
    listener.process(data);
    ASSERT_EQ(callbackCalled, 1);
}

TEST_F(TestGenericCallbackListener, callbackAsClassMethod)
{
    m_callbackData.clear();
    std::vector<std::string> data = {"arg1","arg2"};
    // member function must be annotated with lambda to bind to the instance.
    GenericCallbackListener listener([this](std::vector<std::string> d){this->callback(d); return ProcessInstruction::CONTINUE; });
    listener.process(data);
    ASSERT_EQ(m_callbackData, data);
}

TEST_F(TestGenericCallbackListener, callbackAsNullptr)
{
    GenericCallbackListener listener(nullptr);
    std::vector<std::string> data = {"arg1","arg2"};
    listener.process(data);

}