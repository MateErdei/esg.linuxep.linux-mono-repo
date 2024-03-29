// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "sophos_on_access_process/fanotifyhandler/IFanotifyHandler.h"
#include <gmock/gmock.h>

using namespace testing;

namespace
{
    class MockFanotifyHandler : public sophos_on_access_process::fanotifyhandler::IFanotifyHandler
    {
        public:
        MockFanotifyHandler()
        {
            ON_CALL(*this, getFd).WillByDefault(Return(123));
            ON_CALL(*this, markMount).WillByDefault(Return(0));
            ON_CALL(*this, cacheFd).WillByDefault(Return(0));
            ON_CALL(*this, uncacheFd).WillByDefault(Return(0));
        }
        MOCK_METHOD(int, getFd, (), (const, override));
        MOCK_METHOD(int, markMount, (const std::string& path, bool onOpen, bool onClose), (override));
        MOCK_METHOD(int, unmarkMount, (const std::string& path), (override));
        MOCK_METHOD(int, cacheFd, (const int& fd, const std::string& path, bool), (const, override));
        MOCK_METHOD(int, uncacheFd, (const int& fd, const std::string& path), (const, override));
        MOCK_METHOD(int, clearCachedFiles, (), (const, override));
        MOCK_METHOD(void, init, (), (override));
        MOCK_METHOD(void, close, (), (override));
        MOCK_METHOD(void, updateComplete, (), (override));
    };
}
