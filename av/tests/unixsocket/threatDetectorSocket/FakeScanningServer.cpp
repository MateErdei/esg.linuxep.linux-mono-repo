// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "FakeScanningServer.h"

#include "capnp/serialize.h"
#include "common/StringUtils.h"
#include "scan_messages/ScanRequest.h"
#include "scan_messages/ScanResponse.h"
#include "unixsocket/Logger.h"
#include "unixsocket/SocketUtils.h"
#include "unixsocket/threatDetectorSocket/ThreatDetectedMessageUtils.h"

#include "Common/SystemCallWrapper/SystemCallWrapper.h"

#include <poll.h>

void TestServerConnectionThread::inner_run()
{
    datatypes::AutoFd socket_fd(std::move(m_socketFd));

    while (true)
    {
        struct pollfd fds[]
        {
            { .fd = socket_fd.get(), .events = POLLIN, .revents = 0 }, // socket FD
                { .fd = m_notifyPipe.readFd(), .events = POLLIN, .revents = 0 },
        };
        auto ret = ::ppoll(fds, std::size(fds), nullptr, nullptr);
        assert(ret > 0); std::ignore = ret;
        if ((fds[1].revents & POLLERR) != 0)
        {
            LOGERROR("Error from fds[0]");
            break;
        }
        if ((fds[0].revents & POLLERR) != 0)
        {
            LOGERROR("Error from fds[0]");
            break;
        }
        if ((fds[1].revents & POLLIN) != 0)
        {
            break;
        }

        if ((fds[0].revents & POLLIN) != 0)
        {
            // read length
            auto length = unixsocket::readLength(socket_fd.get());
            if (length == -2)
            {
                LOGDEBUG("Fake Scanning connection thread closed: EOF");
                break;
            }
            else if (length < 0)
            {
                LOGDEBUG("Aborting Fake Scanning connection thread: failed to read length");
                break;
            }
            else if (length == 0)
            {
                LOGDEBUG("Ignoring length of zero / No new messages");
                continue;
            }
            if (!handleEvent(socket_fd, length))
            {
                break;
            }
        }
    }
}

/**
 * Parse a request.
 *
 * Placed in a function to ensure the MessageReader and view are deleted before the buffer might become invalid.
 *
 * @param proto_buffer
 * @param bytes_read
 * @return
 */
static std::shared_ptr<scan_messages::ScanRequest> parseRequest(kj::Array<capnp::word>& proto_buffer, ssize_t& bytes_read)
{
    auto view = proto_buffer.slice(0, bytes_read / sizeof(capnp::word));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::FileScanRequest::Reader requestReader =
        messageInput.getRoot<Sophos::ssplav::FileScanRequest>();

    return std::make_shared<scan_messages::ScanRequest>(requestReader);
}

bool TestServerConnectionThread::handleEvent(datatypes::AutoFd& socket_fd, ssize_t length)
{
    // read capn proto
    uint32_t buffer_size = 256;
    auto proto_buffer = kj::heapArray<capnp::word>(buffer_size);
    auto m_sysCalls = std::make_shared<Common::SystemCallWrapper::SystemCallWrapper>();

    bool loggedLengthOfZero = false;
    scan_messages::ScanResponse result;
    ssize_t bytes_read;
    std::string errMsg;
    if (!readCapnProtoMsg(
            m_sysCalls, length, buffer_size, proto_buffer, socket_fd, bytes_read, loggedLengthOfZero, errMsg,
            std::chrono::milliseconds{10}))
    {
        result.setErrorMsg(errMsg);
        sendResponse(socket_fd, result);
        LOGERROR(errMsg);
        return false;
    }
    LOGDEBUG("Read capn of " << bytes_read);
    std::shared_ptr<scan_messages::ScanRequest> requestReader = parseRequest(proto_buffer, bytes_read);

    std::string escapedPath(requestReader->getPath());
    common::escapeControlCharacters(escapedPath);

    LOGDEBUG("Scan requested of " << escapedPath);

    // read fd
    datatypes::AutoFd file_fd(unixsocket::recv_fd(socket_fd.get()));
    if (file_fd.get() < 0)
    {
        errMsg = "Aborting Scanning connection thread: failed to read fd";
        result.setErrorMsg(errMsg);
        sendResponse(socket_fd, result);
        LOGERROR(errMsg);
        return false;
    }
    LOGDEBUG("Managed to get file descriptor: " << file_fd.get());

    // Keep the connection open if we can read the message but get a file that we can't scan
    if (!isReceivedFdFile(m_sysCalls, file_fd, errMsg) || !isReceivedFileOpen(m_sysCalls, file_fd, errMsg))
    {
        result.setErrorMsg(errMsg);
        sendResponse(socket_fd, result);
        LOGERROR(errMsg);
        return true;
    }

    // Do scan
    file_fd.reset();
    return sendResponse(socket_fd, result);
}

bool TestServerConnectionThread::sendResponse(datatypes::AutoFd& socket_fd, const scan_messages::ScanResponse& response)
{
    if (m_sendGiantResponse)
    {
        size_t total = 1000000000;
        writeLength(socket_fd.get(), total);
        std::string buffer(1000, '\0');
        for (size_t i = 0; i < total / 1000; i++)
        {
            ssize_t bytes_written = ::send(socket_fd.get(), buffer.c_str(), buffer.size(), MSG_NOSIGNAL);
            std::ignore = bytes_written;
        }
        return true;
    }
    else if (!m_nextResponse.empty())
    {
        return sendResponse(socket_fd, m_nextResponse);
    }
    else
    {
        std::string serialised_result = response.serialise();
        return sendResponse(socket_fd, serialised_result);
    }
}

bool TestServerConnectionThread::sendResponse(datatypes::AutoFd& socket_fd, const std::string& serialised_result)
{
    try
    {
        if (!writeLengthAndBuffer(socket_fd.get(), serialised_result))
        {
            LOGWARN("Failed to write result to unix socket");
            return false;
        }
    }
    catch (unixsocket::environmentInterruption& e)
    {
        LOGWARN("Exiting Scanning Connection Thread: " << e.what());
        return false;
    }
    return true;
}

bool TestServerConnectionThread::sendResponse(int socket_fd, size_t length, const std::string& buffer)
{
    writeLength(socket_fd, length);

    ssize_t bytes_written = ::send(socket_fd, buffer.c_str(), buffer.size(), MSG_NOSIGNAL);
    if (static_cast<unsigned>(bytes_written) != buffer.size())
    {
        LOGWARN("Failed to write buffer to unix socket");
        return false;
    }
    return true;
}
