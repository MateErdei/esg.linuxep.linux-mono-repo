// Copyright 2022, Sophos Limited.  All rights reserved.

#include "unixsocket/updateCompleteSocket/UpdateCompleteClientSocketThread.h"

#include "UnixSocketMemoryAppenderUsingTests.h"

#include <gtest/gtest.h>

#include <memory>

using namespace unixsocket::updateCompleteSocket;

namespace
{
    class StubIUpdateCompleteCallback : public IUpdateCompleteCallback
    {
        void updateComplete() override
        {}
    };

    class TestUpdateCompleteClientSocketThread : public UnixSocketMemoryAppenderUsingTests
    {
    };
}

TEST_F(TestUpdateCompleteClientSocketThread, construction)
{
    UpdateCompleteClientSocketThread::IUpdateCompleteCallbackPtr callback;
    callback = std::make_shared<StubIUpdateCompleteCallback>();
    UpdateCompleteClientSocketThread foo("/foo", callback);
}
