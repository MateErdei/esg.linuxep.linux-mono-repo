/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/DirectoryWatcherImpl/DirectoryWatcherImpl.h>
#include <gmock/gmock.h>

using namespace ::testing;

class MockiNotifyWrapper : public Common::DirectoryWatcherImpl::IiNotifyWrapper
{
public:
    MOCK_METHOD0(init, int(void));
    MOCK_METHOD3(addWatch, int(int, const char*, uint32_t));
    MOCK_METHOD2(removeWatch, int(int, int));
    MOCK_METHOD3(read, ssize_t(int, void*, size_t));
};
