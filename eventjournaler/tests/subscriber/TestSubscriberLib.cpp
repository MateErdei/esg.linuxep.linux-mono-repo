/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <modules/SubscriberLib/Subscriber.h>
//#include <Common/Helpers/MockZmqContext.h>
#include "MockZmqContext.h"
#include <Common/FileSystem/IFileSystem.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <Common/Helpers/LogInitializedTests.h>

class TestSubscriber : public LogOffInitializedTests{};

TEST_F(TestSubscriber, binBashShouldExist) // NOLINT
{

    MockZmqContext*  context = new StrictMock<MockZmqContext>();
//    MockZmqContext&  mock(*context);
//    ON_CALL(mock, setTimeout(5000));
    std::shared_ptr<ZMQWrapperApi::IContext>  mockContextPtr(context);

    SubscriberLib::Subscriber subscriber("will not actually create socket", mockContextPtr);
    subscriber.start();
}
