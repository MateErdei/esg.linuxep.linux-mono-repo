/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ThreatReporterServerConnectionThread.h"

#include "ThreatDetected.capnp.h"

#include "unixsocket/Logger.h"
#include "unixsocket/SocketUtils.h"
#include "unixsocket/StringUtils.h"

#include <capnp/serialize.h>
#include <common/FDUtils.h>
#include <scan_messages/ServerThreatDetected.h>

#include <cassert>
#include <stdexcept>
#include <utility>

#include <sys/socket.h>
#include <unistd.h>

using namespace unixsocket;

ThreatReporterServerConnectionThread::ThreatReporterServerConnectionThread(datatypes::AutoFd& fd,
                                                                           std::shared_ptr<IMessageCallback> callback)
        : m_fd(std::move(fd))
        , m_callback(std::move(callback))
{
    if (m_fd < 0)
    {
        throw std::runtime_error("Attempting to construct ThreatReporterServerConnectionThread with invalid socket fd");
    }

    if (!m_callback)
    {
        throw std::runtime_error("Attempting to construct ThreatReporterServerConnectionThread with null callback");
    }
}

//
//static void throwOnError(int ret, const std::string& message)
//{
//    if (ret == 0)
//    {
//        return;
//    }
//    perror(message.c_str());
//    throw std::runtime_error(message);
//}

/**
 * Parse a detection.
 *
 * Placed in a function to ensure the MessageReader and view are deleted before the buffer might become invalid.
 *
 * @param proto_buffer
 * @param bytes_read
 * @return
 */
static scan_messages::ServerThreatDetected parseDetection(kj::Array<capnp::word>& proto_buffer, ssize_t& bytes_read)
{
    auto view = proto_buffer.slice(0, bytes_read / sizeof(capnp::word));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::ThreatDetected::Reader requestReader =
            messageInput.getRoot<Sophos::ssplav::ThreatDetected>();

    if (!requestReader.hasFilePath())
    {
        LOGERROR("Missing file path while parsing report ( size=" << bytes_read << " )");
    }
    return scan_messages::ServerThreatDetected(requestReader);
}

void ThreatReporterServerConnectionThread::run()
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
            LOGFATAL("Terminated ThreatReporterServerConnectionThread with serialisation unimplemented exception: "
                         << ex.getDescription().cStr());
        }
        else
        {
            LOGERROR(
                "Terminated ThreatReporterServerConnectionThread with serialisation exception: "
                    << ex.getDescription().cStr());
        }
    }
    catch (const std::exception& ex)
    {
        LOGERROR("Terminated ThreatReporterServerConnectionThread with exception: " << ex.what());
    }
    catch (...)
    {
        // Fatal since this means we have thrown something that isn't a subclass of std::exception
        LOGFATAL("Terminated ThreatReporterServerConnectionThread with unknown exception");
    }
    setIsRunning(false);
}

void ThreatReporterServerConnectionThread::inner_run()
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
                LOGDEBUG("ThreatReporter Connection closed: EOF");
                break;
            }
            else if (length < 0)
            {
                LOGERROR("Aborting ThreatReporter Connection Thread: failed to read length");
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
            auto detectionReader = parseDetection(proto_buffer, bytes_read);

            if (!detectionReader.hasFilePath())
            {
                LOGERROR("Missing file path in detection report ( size=" << bytes_read << ")");
            }
            else if (detectionReader.getFilePath() == "")
            {
                LOGERROR("Missing file path in detection report: empty file path");
            }
            m_callback->processMessage(generateThreatDetectedXml(detectionReader));

            // TO DO: HANDLE ANY SOCKET ERRORS
        }
    }
}
