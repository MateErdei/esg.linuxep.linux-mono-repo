/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include "Common/ReactorImpl/GenericShutdownListener.h"
using namespace Common::Reactor;

namespace
{
    int callbackCalled = 0;
}


class  TestGenericShutdownListener : public ::testing::Test
{
public:
    std::string m_callbackString;
    void SetUp() override
    {

    }
    void TearDown() override
    {

    }

    void callback()
    {
        m_callbackString = "Callback Called";
    }

};

void pureCallBackFunction()
{
    callbackCalled = 1;
}

TEST_F(TestGenericShutdownListener, callbackAsPureFunction)
{
    callbackCalled = 0;
    GenericShutdownListener listener(pureCallBackFunction);
    listener.notifyShutdownRequested();
    ASSERT_EQ(callbackCalled, 1);
}

TEST_F(TestGenericShutdownListener, callbackAsClassMethod)
{
    m_callbackString = "";
    // member function must be annotated with lambda to bind to the instance.
    GenericShutdownListener listener([this](){this->callback();});
    listener.notifyShutdownRequested();
    ASSERT_EQ(m_callbackString, "Callback Called");
}

TEST_F(TestGenericShutdownListener, callbackAsNullptr)
{
    GenericShutdownListener listener(nullptr);
    listener.notifyShutdownRequested();

}