/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "OtherSideApi.h"

namespace CommsComponent
{
    OtherSideApi::OtherSideApi(std::unique_ptr<AsyncMessager> messager) :
            m_messager(std::move(messager)) {}

    OtherSideApi::~OtherSideApi()
    {
        m_messager->setNotifyClosure([]() {});
    }

    void OtherSideApi::setNotifyOnClosure(NotifySocketClosed notifyOnClosure)
    {
        m_messager->setNotifyClosure(std::move(notifyOnClosure));
    }

    void OtherSideApi::pushMessage(const std::string &message)
    {
        m_messager->sendMessage(message);
    }

    void OtherSideApi::notifyOtherSideAndClose()
    {
        m_messager->setNotifyClosure([]() {});
        m_messager->push_stop();
    }

} 