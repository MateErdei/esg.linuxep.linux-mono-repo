// Copyright 2023, Sophos Limited.  All rights reserved.

#include "modules/unixsocket/BaseClient.h"
#include "modules/unixsocket/SocketUtils.h"

#include <iostream>
#include <cassert>
#include <sstream>

namespace
{
    class TestClient : public unixsocket::BaseClient
    {
    public:
        explicit TestClient(
            std::string socket_path,
            const BaseClient::duration_t& sleepTime= std::chrono::seconds{1},
            BaseClient::IStoppableSleeperSharedPtr sleeper={}) :
            BaseClient(std::move(socket_path), sleepTime, std::move(sleeper))
        {
            unixsocket::BaseClient::connectWithRetries("SafeStore Rescan");
        }

        void sendRequest(const std::string& request)
        {
            assert(m_socket_fd.valid());
            try
            {
                if (!unixsocket::writeLengthAndBuffer(m_socket_fd.get(), request))
                {
                    std::stringstream errMsg;
                    errMsg << "Failed to write to to socket [" << errno << "]";
                    throw std::runtime_error(errMsg.str());
                }
            }
            catch (unixsocket::environmentInterruption& e)
            {
                std::cerr << "Failed to write to socket. Exception caught: " << e.what();
            }
        }

        void sendFD(int fd)
        {
            assert(m_socket_fd.valid());
            try
            {

                if (unixsocket::send_fd(m_socket_fd.get(), fd) < 0)
                {
                    throw std::runtime_error("Failed to write file descriptor to Threat Reporter socket");
                }
            }
            catch (unixsocket::environmentInterruption& e)
            {
                std::cerr << "Failed to write file descriptor to Threat Reporter socket. Exception caught: " << e.what();
            }
        }

        void sendRequestAndFD(const std::string& request, int fd=0)
        {
            sendRequest(request);
            sendFD(fd);
        }

    };
} // namespace