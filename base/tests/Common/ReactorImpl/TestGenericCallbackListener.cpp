/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include "Common/ReactorImpl/GenericCallbackListener.h"
using namespace Common::Reactor;
using namespace Common::ReactorImpl;
using data_t = Common::ZeroMQWrapper::IReadable::data_t;
namespace
{
    int callbackCalled = 0;
}

class TestGenericCallbackListener : public ::testing::Test
{
public:

    data_t m_callbackData;
    void SetUp() override
    {

    }
    void TearDown() override
    {

    }

    void callback(data_t data)
    {
        m_callbackData = data;
    }

};


void pureCallBackFunction(data_t data)
{
    callbackCalled = 1;
}

TEST_F(TestGenericCallbackListener, callbackAsPureFunction)
{
    data_t data = {"arg1","arg2"};
    callbackCalled = 0;
    GenericCallbackListener listener(pureCallBackFunction);
    listener.messageHandler(data);
    ASSERT_EQ(callbackCalled, 1);
}

TEST_F(TestGenericCallbackListener, callbackAsClassMethod)
{
    m_callbackData.clear();
    data_t data = {"arg1","arg2"};
    // member function must be annotated with lambda to bind to the instance.
    GenericCallbackListener listener([this](data_t d){this->callback(d);});
    listener.messageHandler(data);
    ASSERT_EQ(m_callbackData, data);
}

TEST_F(TestGenericCallbackListener, callbackAsNullptr)
{
    GenericCallbackListener listener(nullptr);
    data_t data = {"arg1","arg2"};
    listener.messageHandler(data);

}