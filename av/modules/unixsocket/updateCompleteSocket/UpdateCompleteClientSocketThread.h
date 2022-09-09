// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "IUpdateCompleteCallback.h"

#include "unixsocket/BaseClient.h"

#include "common/AbstractThreadPluginInterface.h"

#include <memory>

namespace unixsocket::updateCompleteSocket
{
    class UpdateCompleteClientSocketThread : public unixsocket::BaseClient,
                                             public common::AbstractThreadPluginInterface
    {
    public:
        using IUpdateCompleteCallbackPtr = std::shared_ptr<IUpdateCompleteCallback>;
        UpdateCompleteClientSocketThread(std::string socket_path,
                                         IUpdateCompleteCallbackPtr callback,
                                         struct timespec reconnectInterval={1,0}
            );

        void run() override;
        bool connected();
    private:
        void connectIfNotConnected();

        IUpdateCompleteCallbackPtr m_callback;
        struct timespec m_reconnectInterval;
    };
}
