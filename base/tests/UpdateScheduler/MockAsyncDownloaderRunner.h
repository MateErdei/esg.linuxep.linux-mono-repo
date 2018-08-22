/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>
#include "gmock/gmock.h"
#include <UpdateSchedulerImpl/IAsyncSulDownloaderRunner.h>

using namespace ::testing;


class MockAsyncDownloaderRunner
        : public UpdateSchedulerImpl::IAsyncSulDownloaderRunner
{
public:
    MOCK_METHOD0(triggerSulDownloader, void());

    MOCK_METHOD0(isRunning, bool());

    MOCK_METHOD0(triggerAbort, void());
};
