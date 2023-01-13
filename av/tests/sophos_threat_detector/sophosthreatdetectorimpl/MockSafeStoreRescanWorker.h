// Copyright 2023, Sophos Limited.  All rights reserved.

#pragma once

#include <sophos_threat_detector/sophosthreatdetectorimpl/ISafeStoreRescanWorker.h>
#include <gmock/gmock.h>

using namespace testing;

namespace
{
    class MockSafeStoreRescanWorker : public sspl::sophosthreatdetectorimpl::ISafeStoreRescanWorker
    {
        public:
            MockSafeStoreRescanWorker() {}

            MOCK_METHOD(void, triggerRescan, (),(override));
            MOCK_METHOD(void, sendRescanRequest, (), (override));

            void tryStop() override
            {
                Common::Threads::AbstractThread::requestStop();
            }

            void run() override
            {
                announceThreadStarted();
            }
    };
}
