/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "gmock/gmock.h"

#include <watchdog/watchdogimpl/IWatchdogRequest.h>
#include <watchdog/watchdogimpl/WatchdogServiceException.h>

#include <string>

using namespace ::testing;

class MockIWatchdogRequest : public watchdog::watchdogimpl::IWatchdogRequest
{
public:
    MOCK_METHOD0(requestUpdateService, void());
};

class IWatchdogRequestReplacement
{
public:
    explicit IWatchdogRequestReplacement(
        std::function<std::unique_ptr<watchdog::watchdogimpl::IWatchdogRequest>()> func)
    {
        watchdog::watchdogimpl::factory().replace(func);
    }

    explicit IWatchdogRequestReplacement() :
        IWatchdogRequestReplacement([]() {
            MockIWatchdogRequest* mock = new StrictMock<MockIWatchdogRequest>();
            EXPECT_CALL(*mock, requestUpdateService);
            return std::unique_ptr<watchdog::watchdogimpl::IWatchdogRequest>(mock);
        })
    {
    }
    explicit IWatchdogRequestReplacement(std::string error) :
        IWatchdogRequestReplacement([error]() {
            MockIWatchdogRequest* mock = new StrictMock<MockIWatchdogRequest>();
            EXPECT_CALL(*mock, requestUpdateService).WillOnce(Invoke([error]() {
                throw watchdog::watchdogimpl::WatchdogServiceException(error);
            }));
            return std::unique_ptr<watchdog::watchdogimpl::IWatchdogRequest>(mock);
        })
    {
    }

    ~IWatchdogRequestReplacement() { watchdog::watchdogimpl::factory().restore(); }
};
