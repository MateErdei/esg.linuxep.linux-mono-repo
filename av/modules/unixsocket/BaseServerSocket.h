/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#define AUTO_FD_IMPLICIT_INT
#include "datatypes/AutoFd.h"
#include "Common/Threads/AbstractThread.h"

#include <string>
#include <vector>

namespace unixsocket
{
    class BaseServerSocket  : public Common::Threads::AbstractThread
    {

    public:
        BaseServerSocket(const BaseServerSocket&) = delete;
        BaseServerSocket& operator=(const BaseServerSocket&) = delete;

        explicit BaseServerSocket(const std::string& path);
        ~BaseServerSocket() override;

        void run() override;

    private:
        std::string m_path;

    protected:
        /**
         * Handle a new connection.
         * @param fd
         * @return True if we should terminate.
         */

        virtual bool handleConnection(int fd) = 0;
        virtual void killThreads() = 0;

        datatypes::AutoFd m_socket_fd;
    };


    template <typename T>
    class ImplServerSocket : public BaseServerSocket
    {
    public:
        using BaseServerSocket::BaseServerSocket;

    protected:
        using TPtr = std::unique_ptr<T>;

        bool handleConnection(int fd) override
        {
            auto thread = std::make_unique<T>(fd);
            thread->start();
            m_threadVector.emplace_back(std::move(thread));
            return false;
        }

        void killThreads() override
        {
            for (auto& thread : m_threadVector)
            {
                thread->requestStop();
            }
            for (auto& thread : m_threadVector)
            {
                thread->join();
            }
            m_threadVector.clear();
        }

    private:
        std::vector<TPtr> m_threadVector;
    };
}
