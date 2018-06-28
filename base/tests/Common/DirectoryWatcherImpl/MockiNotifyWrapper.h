/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_MOCKINOTIFYWRAPPER_H
#define EVEREST_BASE_MOCKINOTIFYWRAPPER_H

#include "gmock/gmock.h"
#include "Common/DirectoryWatcher/IDirectoryWatcher.h"

using namespace ::testing;

class MockiNotifyWrapper : public Common::DirectoryWatcherImpl::IiNotifyWrapper
{
public:
    MOCK_METHOD0(init, int(void));
    MOCK_METHOD3(addWatch, int(int, const char *, uint32_t));
    MOCK_METHOD2(removeWatch, int(int, int));
    MOCK_METHOD3(read, ssize_t(int, void*, size_t));
};

#endif //EVEREST_BASE_MOCKINOTIFYWRAPPER_H
