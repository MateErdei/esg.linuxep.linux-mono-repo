// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "ProcessControllerServerConnectionThread.h"

#include "ProcessControl.capnp.h"

#include "unixsocket/Logger.h"
#include "unixsocket/SocketUtils.h"

#include <capnp/serialize.h>
#include <common/FDUtils.h>
#include <common/SaferStrerror.h>
#include <scan_messages/ProcessControlDeserialiser.h>

#include <cassert>
#include <stdexcept>
#include <utility>

#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace unixsocket;

ProcessControllerServerConnectionThread::ProcessControllerServerConnectionThread(datatypes::AutoFd& fd,
                                                                                 std::shared_ptr<IProcessControlMessageCallback> processControlCallback,
                                                                                 datatypes::ISystemCallWrapperSharedPtr sysCalls)
    : m_fd(std::move(fd))
    , m_controlMessageCallback(std::move(processControlCallback))
    , m_sysCalls(sysCalls)
{
    if (m_fd < 0)
    {
        throw std::runtime_error("Attempting to construct ProcessControllerServerConnectionThread with invalid socket fd");
    }
}

/**
 * Parse a control request.
 *
 * Placed in a function to ensure the MessageReader and view are deleted before the buffer might become invalid.
 *
 * @param proto_buffer
 * @param bytes_read
 * @return
 */
static scan_messages::ProcessControlDeserialiser parseProcessControlRequest(kj::Array<capnp::word>& proto_buffer, ssize_t& bytes_read)
{
    auto view = proto_buffer.slice(0, bytes_read / sizeof(capnp::word));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::ProcessControl::Reader requestReader =
            messageInput.getRoot<Sophos::ssplav::ProcessControl>();

    return scan_messages::ProcessControlDeserialiser(requestReader);
}

void ProcessControllerServerConnectionThread::run()
{
    setIsRunning(true);
    announceThreadStarted();

    try
    {
        inner_run();
    }
    catch (const kj::Exception& ex)
    {
        if (ex.getType() == kj::Exception::Type::UNIMPLEMENTED)
        {
            // Fatal since this means we have a coding error that calls something unimplemented in kj.
            LOGFATAL("Terminated ProcessControllerServerConnectionThread with serialisation unimplemented exception: "
                         << ex.getDescription().cStr());
        }
        else
        {
            LOGERROR(
                "Terminated ProcessControllerServerConnectionThread with serialisation exception: "
                    << ex.getDescription().cStr());
        }
    }
    catch (const std::exception& ex)
    {
        LOGERROR("Terminated ProcessControllerServerConnectionThread with exception: " << ex.what());
    }
    catch (...)
    {
        // Fatal since this means we have thrown something that isn't a subclass of std::exception
        LOGFATAL("Terminated ProcessControllerServerConnectionThread with unknown exception");
    }
    setIsRunning(false);
}

void ProcessControllerServerConnectionThread::inner_run()
{
    datatypes::AutoFd socket_fd(std::move(m_fd));
    LOGDEBUG("Process Controller Server thread got connection " << socket_fd.fd());
    uint32_t buffer_size = 512;
    auto proto_buffer = kj::heapArray<capnp::word>(buffer_size);

    struct pollfd fds[] {
        { .fd = socket_fd.get(), .events = POLLIN, .revents = 0 }, // socket FD
        { .fd = m_notifyPipe.readFd(), .events = POLLIN, .revents = 0 },
    };
    bool loggedLengthOfZero = false;

    while (true)
    {
        auto ret = m_sysCalls->ppoll(fds, std::size(fds), nullptr, nullptr);
        if (ret < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            LOGFATAL("Error from ppoll: " << common::safer_strerror(errno));
            return;
        }
        else if (ret > 0)
        {
            if ((fds[1].revents & POLLERR) != 0)
            {
                LOGERROR("Closing Process Controller connection thread, error from notify pipe: " << common::safer_strerror(errno));
                break;
            }

            if ((fds[0].revents & POLLERR) != 0)
            {
                LOGERROR("Closing Process Controller connection thread, error from socket: " << common::safer_strerror(errno));
                break;
            }

            if ((fds[1].revents & POLLIN) != 0)
            {
                LOGSUPPORT("Closing Process Controller connection thread");
                break;
            }

            if ((fds[0].revents & POLLIN) != 0)
            {
                // read length
                int32_t length = unixsocket::readLength(socket_fd);
                if (length == -2)
                {
                    LOGDEBUG("Process Controller connection thread closed: EOF");
                    break;
                }
                else if (length < 0)
                {
                    LOGERROR("Aborting Process Controller connection thread: failed to read length");
                    break;
                }
                else if (length == 0)
                {
                    if (not loggedLengthOfZero)
                    {
                        LOGDEBUG("Ignoring length of zero / No new messages");
                        loggedLengthOfZero = true;
                    }
                    continue;
                }

                // read capn proto
                if (static_cast<uint32_t>(length) > (buffer_size * sizeof(capnp::word)))
                {
                    buffer_size = 1 + length / sizeof(capnp::word);
                    proto_buffer = kj::heapArray<capnp::word>(buffer_size);
                    loggedLengthOfZero = false;
                }

                ssize_t bytes_read = ::read(socket_fd, proto_buffer.begin(), length);
                if (bytes_read < 0)
                {
                    LOGERROR("Process Controller connection thread aborting socket connection: " << errno);
                    break;
                }
                else if (bytes_read != length)
                {
                    LOGERROR("Process Controller connection thread aborting socket connection: failed to read entire message");
                    break;
                }

                LOGDEBUG("Read capn of " << bytes_read);
                auto processControlReader = parseProcessControlRequest(proto_buffer, bytes_read);

                m_controlMessageCallback->processControlMessage(processControlReader.getCommandType());
            }
        }
    }
}
