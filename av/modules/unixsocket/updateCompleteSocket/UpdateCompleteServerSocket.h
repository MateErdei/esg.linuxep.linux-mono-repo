// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "unixsocket/BaseServerSocket.h"

#include "sophos_threat_detector/threat_scanner/IUpdateCompleteCallback.h"

#include <mutex>

namespace unixsocket::updateCompleteSocket
{
    class UpdateCompleteServerSocket : public BaseServerSocket,
                                       public threat_scanner::IUpdateCompleteCallback
    {
    public:
        using ConnectionVector = std::vector<datatypes::AutoFd>;
        explicit UpdateCompleteServerSocket(const sophos_filesystem::path& path, mode_t mode);
        /**
         * Publish that we have completed an update
         */
        void updateComplete() override;
        void publishUpdateComplete();

        [[nodiscard]] ConnectionVector::size_type clientCount() const;

    protected:

        bool handleConnection(datatypes::AutoFd& fd) override;

        /**
         * No actual threads, but we want to clear connections.
         */
        void killThreads() override;

    private:
        bool trySendUpdateComplete(datatypes::AutoFd& fd);
        ConnectionVector m_connections;
        mutable std::mutex m_connectionsLock;
    };
}
