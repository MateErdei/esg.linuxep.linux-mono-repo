/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/DirectoryWatcher/IiNotifyWrapper.h>

#include <gmock/gmock.h>

using namespace ::testing;

class MockiNotifyWrapper : public Common::DirectoryWatcher::IiNotifyWrapper
{
public:
    MOCK_METHOD0(init, int(void));
    MOCK_METHOD3(addWatch, int(int, const char*, uint32_t));
    MOCK_METHOD2(removeWatch, int(int, int));
    MOCK_METHOD3(read, ssize_t(int, void*, size_t));
};
