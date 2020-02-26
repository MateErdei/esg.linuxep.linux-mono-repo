/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#define AUTO_FD_IMPLICIT_INT
#include "datatypes/AutoFd.h"
#include "Common/Threads/AbstractThread.h"

#include <string>

namespace unixsocket
{
    class BaseServerSocket  : public Common::Threads::AbstractThread
    {

    public:
        BaseServerSocket(const BaseServerSocket&) = delete;
        BaseServerSocket& operator=(const BaseServerSocket&) = delete;

        explicit BaseServerSocket(const std::string& path);

        void run() override;

    protected:
        /**
         * Handle a new connection.
         * @param fd
         * @return True if we should terminate.
         */
        virtual bool handleConnection(int fd) = 0;
        datatypes::AutoFd m_socket_fd;
    };


    template <typename T>
    class ImplServerSocket : public BaseServerSocket
    {
    public:
        using BaseServerSocket::BaseServerSocket;

    protected:
        bool handleConnection(int fd) override
        {
            T thread(fd);
            thread.run();
            return false;
        }
    };
}
