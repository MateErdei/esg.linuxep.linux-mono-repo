// Copyright 2022, Sophos Limited.  All rights reserved.

#include "UpdateCompleteClientSocketThread.h"

#include <utility>
unixsocket::updateCompleteSocket::UpdateCompleteClientSocketThread::UpdateCompleteClientSocketThread(
    std::string socket_path,
    unixsocket::updateCompleteSocket::UpdateCompleteClientSocketThread::IUpdateCompleteCallbackPtr callback)
    :
    BaseClient(std::move(socket_path)),
    m_callback(std::move(callback))
{
}

void unixsocket::updateCompleteSocket::UpdateCompleteClientSocketThread::run()
{
    announceThreadStarted();
}
