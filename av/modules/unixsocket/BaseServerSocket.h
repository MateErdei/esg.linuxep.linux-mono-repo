/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#define AUTO_FD_IMPLICIT_INT

#include "IMessageCallback.h"
#include "IMonitorable.h"

#include "datatypes/AutoFd.h"
#include "datatypes/sophos_filesystem.h"

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
        ~BaseServerSocket() override;

    protected:
        /**
         * Constructor used by implementation classes
         * @param path
         * @param mode
         */
        explicit BaseServerSocket(const sophos_filesystem::path& path, mode_t mode);

    public:
        void run() override;

    private:
        std::string m_socketPath;

    protected:
        /**
         * Handle a new connection.
         * @param fd
         * @return True if we should terminate.
         */

        virtual bool handleConnection(datatypes::AutoFd& fd) = 0;
        virtual void killThreads() = 0;

        datatypes::AutoFd m_socket_fd;
    };


    template <typename T>
    class ImplServerSocket : public BaseServerSocket
    {
    protected:
        /**
         * Inherit constructors
         */
        using BaseServerSocket::BaseServerSocket;
        using TPtr = std::unique_ptr<T>;

        virtual TPtr makeThread(datatypes::AutoFd& fd) = 0;

        bool handleConnection(datatypes::AutoFd& fd) override
        {
            // first, tidy up any stale threads
            for (auto it = m_threadVector.begin(); it != m_threadVector.end(); )
            {
                TPtr& thread = *it;
                if ( ! thread->isRunning() )
                {
                    thread->join();
                    it = m_threadVector.erase(it);
                }
                else
                {
                    ++it;
                }
            }

            auto thread = makeThread(fd);
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

        std::vector<TPtr> m_threadVector;
    };
}
