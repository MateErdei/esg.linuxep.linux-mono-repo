/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#define AUTO_FD_IMPLICIT_INT

#include "IMessageCallback.h"

#include "datatypes/AutoFd.h"
#include "datatypes/sophos_filesystem.h"

#include "common/AbstractThreadPluginInterface.h"

#include <common/ErrorCodes.h>

#include <string>
#include <vector>

static const int MAX_CLIENT_CONNECTIONS = 128;

namespace unixsocket
{
    class BaseServerSocket  : public common::AbstractThreadPluginInterface
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
        [[nodiscard]] int getReturnCode() const
        {
            return m_returnCode;
        }

        [[nodiscard]] int maxClientConnections() const
        {
            return m_max_threads;
        }

    private:
        std::string m_socketPath;
        int m_returnCode = common::E_CLEAN_SUCCESS;

    protected:
        /**
         * Handle a new connection.
         * @param fd
         * @return True if we should terminate.
         */
        virtual bool handleConnection(datatypes::AutoFd& fd) = 0;

        void logError(const std::string&);
        void logDebug(const std::string&);

        datatypes::AutoFd m_socket_fd;
        static const int m_max_threads = MAX_CLIENT_CONNECTIONS;
        std::string m_socketName = "Base Server Socket";

        /**
         * Kill any extra threads started to handle incoming connections
         */
        virtual void killThreads() = 0;
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
        virtual void logMaxConnectionsError() = 0;

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

            if (m_threadVector.size() >= m_max_threads)
            {
                fd.close();
                logMaxConnectionsError();
                return false;
            }
            else
            {
                std::ostringstream ost;
                ost << m_socketName << " accepting connection: fd = "
                    << fd.get()
                    << " "
                    << m_threadVector.size() << " / " << m_max_threads;
                logDebug(ost.str());
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
