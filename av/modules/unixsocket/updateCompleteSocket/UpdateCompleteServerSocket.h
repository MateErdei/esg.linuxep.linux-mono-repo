// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "unixsocket/BaseServerSocket.h"

namespace unixsocket::updateCompleteSocket
{
    class UpdateCompleteServerSocket : public BaseServerSocket
    {
    public:
        /**
         * Publish that we have completed an update
         */
        void publishUpdateComplete();

        bool handleConnection(datatypes::AutoFd& fd) override;
    protected:
        void killThreads() override
        {}

    private:
        bool trySendUpdateComplete(datatypes::AutoFd& fd);
        std::vector<datatypes::AutoFd> m_connections;
    };
}
