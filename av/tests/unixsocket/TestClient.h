// Copyright 2023 Sophos Limited. All rights reserved.

#include "common/SaferStrerror.h"
#include "unixsocket/BaseClient.h"
#include "unixsocket/SocketUtils.h"

#include "Common/Exceptions/IException.h"
#include "Common/SystemCallWrapper/SystemCallWrapper.h"

#include <cassert>
#include <iostream>
#include <sstream>

namespace
{
    class TestClientException : public Common::Exceptions::IException
    {
    public:
        using Common::Exceptions::IException::IException;
    };

    class SendFDException : public TestClientException
    {
    public:
        int error_;
        SendFDException(const char* file, const unsigned long line, const std::string& what, int error) :
            TestClientException(file, line, what),error_(error)
        {
        }
    };

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
                Common::SystemCallWrapper::SystemCallWrapper systemCallWrapper;
                if (!unixsocket::writeLengthAndBuffer(systemCallWrapper, m_socket_fd.get(), request))
                {
                    std::stringstream errMsg;
                    errMsg << "Failed to write to to socket [" << errno << "]";
                    throw Common::Exceptions::IException(LOCATION, errMsg.str());
                }
            }
            catch (const unixsocket::EnvironmentInterruption& e)
            {
                std::cerr << "Failed to write to socket. Exception caught: " << e.what()
                          << " at " << e.where_ << '\n';
            }
        }

        void sendFD(int fd)
        {
            assert(m_socket_fd.valid());
            assert(fd >= 0);
            try
            {
                if (unixsocket::send_fd(m_socket_fd.get(), fd) < 0)
                {
                    int error = errno;
                    std::stringstream errMsg;
                    errMsg << "Failed to write file descriptor to Threat Reporter socket fd="
                           << std::to_string(fd)
                           << " errno=" << error
                           << " error str=" << common::safer_strerror(error);
                    throw SendFDException(LOCATION, errMsg.str(), error);
                }
            }
            catch (const unixsocket::EnvironmentInterruption& e)
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