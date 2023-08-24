// Copyright 2018-2023 Sophos Limited. All rights reserved.
#pragma once

#include "Common/WatchdogRequest/IWatchdogRequest.h"
#include "Common/WatchdogRequest/WatchdogServiceException.h"

#include <gmock/gmock.h>

#include <string>

using namespace ::testing;

class MockIWatchdogRequest : public Common::WatchdogRequest::IWatchdogRequest
{
public:
    MOCK_METHOD0(requestUpdateService, void());
    MOCK_METHOD0(requestDiagnoseService, void());
};

class IWatchdogRequestReplacement
{
public:
    explicit IWatchdogRequestReplacement(
        std::function<std::unique_ptr<Common::WatchdogRequest::IWatchdogRequest>()> func)
    {
        Common::WatchdogRequest::factory().replace(func);
    }

    explicit IWatchdogRequestReplacement() :
        IWatchdogRequestReplacement(
            []()
            {
                auto mock = std::make_unique<StrictMock<MockIWatchdogRequest>>();
                EXPECT_CALL(*mock, requestUpdateService);
                return mock;
            })
    {
    }
    explicit IWatchdogRequestReplacement(std::string error) :
        IWatchdogRequestReplacement(
            [error]()
            {
                auto mock = std::make_unique<StrictMock<MockIWatchdogRequest>>();
                EXPECT_CALL(*mock, requestUpdateService)
                    .WillOnce(Invoke([error]() { throw Common::WatchdogRequest::WatchdogServiceException(error); }));
                return mock;
            })
    {
    }

    ~IWatchdogRequestReplacement() { Common::WatchdogRequest::factory().restore(); }
};
