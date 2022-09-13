//Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include <sophos_on_access_process/fanotifyhandler/IFanotifyHandler.h>
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
        }
        MOCK_METHOD(int, getFd, (), (const));
        MOCK_METHOD(int, markMount, (const unsigned int& flags, const uint64_t& mask, const int& dfd, const std::string& path), (const));
        MOCK_METHOD(int, cacheFd, (const unsigned int& flags, const uint64_t& mask, const int& dfd, const std::string& path), (const));
    };
}