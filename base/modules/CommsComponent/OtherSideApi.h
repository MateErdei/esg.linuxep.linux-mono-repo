/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once
#include <string>
#include "AsyncMessager.h"
#include "IOtherSideApi.h"

namespace CommsComponent
{
    class OtherSideApi : public IOtherSideApi
    {
    public:
        explicit OtherSideApi(std::unique_ptr<AsyncMessager> messager);

        ~OtherSideApi();

        virtual void pushMessage(const std::string &);

        void notifyOtherSideAndClose();

        void setNotifyOnClosure(NotifySocketClosed);

    private:
        std::unique_ptr<AsyncMessager> m_messager;
    };
}