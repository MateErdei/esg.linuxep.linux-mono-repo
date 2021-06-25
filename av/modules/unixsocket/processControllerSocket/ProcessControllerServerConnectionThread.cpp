/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ProcessControllerServerConnectionThread.h"

#include "ProcessControl.capnp.h"

#include "unixsocket/Logger.h"
#include "unixsocket/SocketUtils.h"

#include <capnp/serialize.h>
#include <common/FDUtils.h>
#include <scan_messages/ProcessControlDeserialiser.h>

#include <cassert>
#include <stdexcept>
#include <utility>

#include <sys/socket.h>
#include <unistd.h>

using namespace unixsocket;

ProcessControllerServerConnectionThread::ProcessControllerServerConnectionThread(datatypes::AutoFd& fd,
                                                                                 std::shared_ptr<Common::Threads::NotifyPipe> shutdownPipe)
        : m_fd(std::move(fd))
        , m_shutdownPipe(std::move(shutdownPipe))
{
    if (m_fd < 0)
    {
        throw std::runtime_error("Attempting to construct ProcessControllerServerConnectionThread with invalid socket fd");
    }

    if (m_shutdownPipe.get() == nullptr)
    {
        throw std::runtime_error("Attempting to construct ProcessControllerServerConnectionThread with null shutdown pipe");
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
    LOGDEBUG("Got connection " << socket_fd.fd());
    uint32_t buffer_size = 512;
    auto proto_buffer = kj::heapArray<capnp::word>(buffer_size);

    int exitFD = m_notifyPipe.readFd();

    fd_set readFDs;
    FD_ZERO(&readFDs);
    int max = -1;
    max = FDUtils::addFD(&readFDs, exitFD, max);
    max = FDUtils::addFD(&readFDs, socket_fd, max);
    bool loggedLengthOfZero = false;

    while (true)
    {
        fd_set tempRead = readFDs;

        int activity = ::pselect(max + 1, &tempRead, nullptr, nullptr, nullptr, nullptr);
        if (activity < 0)
        {

            LOGERROR("Closing socket, error: " << errno);
            break;
        }

        if (FDUtils::fd_isset(exitFD, &tempRead))
        {
            LOGSUPPORT("Closing scanning socket thread");
            break;
        }
        else
        // if (FDUtils::fd_isset(socket_fd, &tempRead))
        {
            // If shouldn't be required - we have no timeout, and only 2 FDs in the pselect.
            // exitFD will cause break
            // therefore "else" must be fd_isset(socket_fd, &tempRead)
            assert(FDUtils::fd_isset(socket_fd, &tempRead));

            // read length
            int32_t length = unixsocket::readLength(socket_fd);
            if (length == -2)
            {
                LOGDEBUG("ProcessController Connection closed: EOF");
                break;
            }
            else if (length < 0)
            {
                LOGERROR("Aborting ProcessController Connection Thread: failed to read length");
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
                LOGERROR("Aborting socket connection: " << errno);
                break;
            }
            else if (bytes_read != length)
            {
                LOGERROR("Aborting socket connection: failed to read entire message");
                break;
            }

            LOGDEBUG("Read capn of " << bytes_read);
            auto processControlReader = parseProcessControlRequest(proto_buffer, bytes_read);
            if (processControlReader.getCommandType() == scan_messages::E_SHUTDOWN)
            {
                m_shutdownPipe->notify();
                break;
            }
            else
            {
                LOGDEBUG("Received unknown signal on Process Controller: " << processControlReader.getCommandType());
            }
        }
    }
}
