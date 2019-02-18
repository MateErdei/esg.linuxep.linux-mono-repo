/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "gmock/gmock.h"

#include <UpdateScheduler/IAsyncSulDownloaderRunner.h>

#include <string>

using namespace ::testing;

class MockAsyncDownloaderRunner : public UpdateScheduler::IAsyncSulDownloaderRunner
{
public:
    MOCK_METHOD0(triggerSulDownloader, void());

    MOCK_METHOD0(isRunning, bool());

    MOCK_METHOD0(triggerAbort, void());
};
