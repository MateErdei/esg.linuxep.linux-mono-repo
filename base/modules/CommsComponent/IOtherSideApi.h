/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once
#include <string>
#include "AsyncMessager.h"
namespace CommsComponent
{
    class IOtherSideApi
    {
    public:
        virtual ~IOtherSideApi() = default;

        virtual void pushMessage(const std::string &) = 0;

        virtual void notifyOtherSideAndClose() = 0;

        virtual void setNotifyOnClosure(NotifySocketClosed) = 0;
    };
}