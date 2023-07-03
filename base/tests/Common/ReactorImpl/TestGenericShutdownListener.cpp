/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Common/ReactorImpl/GenericShutdownListener.h"

#include <gtest/gtest.h>
using namespace Common::Reactor;
using namespace Common::ReactorImpl;
namespace
{
    int callbackCalled = 0;
}

class TestGenericShutdownListener : public ::testing::Test
{
public:
    std::string m_callbackString;
    void SetUp() override
    {
        m_callbackString = "";
        callbackCalled = 0;
    }
    void TearDown() override {}

    void callback() { m_callbackString = "Callback Called"; }
};

void pureCallBackFunction()
{
    callbackCalled = 1;
}

TEST_F(TestGenericShutdownListener, callbackAsPureFunction) // NOLINT
{
    GenericShutdownListener listener(pureCallBackFunction);
    listener.notifyShutdownRequested();
    ASSERT_EQ(callbackCalled, 1);
}

TEST_F(TestGenericShutdownListener, callbackAsClassMethod) // NOLINT
{
    // member function must be annotated with lambda to bind to the instance.
    GenericShutdownListener listener([this]() { this->callback(); });
    listener.notifyShutdownRequested();
    ASSERT_EQ(m_callbackString, "Callback Called");
}

TEST_F(TestGenericShutdownListener, callbackAsNullptr) // NOLINT
{
    GenericShutdownListener listener(nullptr);
    listener.notifyShutdownRequested();
}