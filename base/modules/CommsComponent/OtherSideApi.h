/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once
#include <string>
#include "AsyncMessager.h"
namespace CommsComponent
{
    class OtherSideApi
    {
    public:
        explicit OtherSideApi(std::unique_ptr<AsyncMessager> messager);

        ~OtherSideApi();

        void pushMessage(const std::string &);

        void notifyOtherSideAndClose();

        void setNofifyOnClosure(NofifySocketClosed);

    private:
        std::unique_ptr<AsyncMessager> m_messager;
    };
}