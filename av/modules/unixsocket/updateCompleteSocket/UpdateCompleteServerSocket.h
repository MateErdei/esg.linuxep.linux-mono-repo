// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "unixsocket/BaseServerSocket.h"

namespace unixsocket::updateCompleteSocket
{
    class UpdateCompleteServerSocket : public BaseServerSocket
    {
    public:
        explicit UpdateCompleteServerSocket(const sophos_filesystem::path& path, mode_t mode);
        /**
         * Publish that we have completed an update
         */
        void publishUpdateComplete();

        [[nodiscard]] int clientCount() const;

    protected:
        void killThreads() override
        {}

        bool handleConnection(datatypes::AutoFd& fd) override;

    private:
        bool trySendUpdateComplete(datatypes::AutoFd& fd);
        std::vector<datatypes::AutoFd> m_connections;
    };
}
