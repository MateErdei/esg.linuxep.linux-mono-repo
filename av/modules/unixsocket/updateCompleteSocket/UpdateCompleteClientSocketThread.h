// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "sophos_threat_detector/threat_scanner/IUpdateCompleteCallback.h"

#include "unixsocket/BaseClient.h"

#include "common/AbstractThreadPluginInterface.h"

#include <memory>

namespace unixsocket::updateCompleteSocket
{
    class UpdateCompleteClientSocketThread : public unixsocket::BaseClient,
                                             public common::AbstractThreadPluginInterface
    {
    public:
        UpdateCompleteClientSocketThread(std::string socket_path,
                                         threat_scanner::IUpdateCompleteCallbackPtr callback,
                                         struct timespec reconnectInterval={1,0}
            );

        void run() override;
        bool connected();
    private:
        void connectIfNotConnected();

        threat_scanner::IUpdateCompleteCallbackPtr m_callback;
        struct timespec m_reconnectInterval;
    };
}
