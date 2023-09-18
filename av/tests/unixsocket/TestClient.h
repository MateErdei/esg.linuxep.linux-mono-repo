// Copyright 2023 Sophos Limited. All rights reserved.

#include "unixsocket/BaseClient.h"
#include "unixsocket/SocketUtils.h"

#include "Common/Exceptions/IException.h"

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
            BaseClient(std::move(socket_path),"SafeStore Rescan", sleepTime, std::move(sleeper))
        {
            unixsocket::BaseClient::connectWithRetries();
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
                    throw Common::Exceptions::IException(LOCATION, errMsg.str());
                }
            }
            catch (const unixsocket::environmentInterruption& e)
            {
                std::cerr << "Failed to write to socket. Exception caught: " << e.what()
                          << " at " << e.where_ << '\n';
            }
        }

        void sendFD(int fd)
        {
            assert(m_socket_fd.valid());
            try
            {

                if (unixsocket::send_fd(m_socket_fd.get(), fd) < 0)
                {
                    std::stringstream errMsg;
                    errMsg << "Failed to write file descriptor to Threat Reporter socket fd="
                           << std::to_string(fd)
                           << " errno=" << errno;
                    throw Common::Exceptions::IException(LOCATION, errMsg.str());
                }
            }
            catch (unixsocket::environmentInterruption& e)
            {
                std::cerr << "Failed to write file descriptor to Threat Reporter socket. Exception caught: " << e.what()
                        << " at " << e.where_ << '\n';
            }
        }

        void sendRequestAndFD(const std::string& request, int fd=0)
        {
            sendRequest(request);
            sendFD(fd);
        }

    };
} // namespace