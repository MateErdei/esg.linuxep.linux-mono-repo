// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "ThreatDetectedMessageUtils.h"

#include "common/SaferStrerror.h"
#include "unixsocket/SocketUtils.h"

#include <sstream>

bool unixsocket::isReceivedFdFile(
    Common::SystemCallWrapper::ISystemCallWrapperSharedPtr  sysCallWrapper,
    datatypes::AutoFd& file_fd,
    std::string& errMsg)
{
    struct ::stat st
    {
    };
    int ret = sysCallWrapper->fstat(file_fd.get(), &st);
    if (ret == -1)
    {
        std::stringstream errSS;
        errSS << "Failed to get status of received file FD: " << common::safer_strerror(errno);
        errMsg = errSS.str();
        return false;
    }
    if (!S_ISREG(st.st_mode))
    {
        errMsg = "Received file FD is not a regular file";
        return false;
    }
    return true;
}

bool unixsocket::isReceivedFileOpen(
    Common::SystemCallWrapper::ISystemCallWrapperSharedPtr  sysCallWrapper,
    datatypes::AutoFd& file_fd,
    std::string& errMsg)
{
    int status = sysCallWrapper->fcntl(file_fd.get(), F_GETFL);
    if (status == -1)
    {
        std::stringstream errSS;
        errSS << "Failed to get status flags of received file FD: " << common::safer_strerror(errno);
        errMsg = errSS.str();
        return false;
    }
    unsigned int mode = status & O_ACCMODE;
    if (!(mode == O_RDONLY || mode == O_RDWR ) || status & O_PATH )
    {
        errMsg = "Received file FD is not open for read";
        return false;
    }
    return true;
}

bool unixsocket::readCapnProtoMsg(
    const Common::SystemCallWrapper::ISystemCallWrapperSharedPtr & sysCallWrapper,
    ssize_t length,
    uint32_t& buffer_size,
    kj::Array<capnp::word>& proto_buffer,
    datatypes::AutoFd& socket_fd,
    ssize_t& bytes_read,
    bool& loggedLengthOfZero,
    std::string& errMsg,
    std::chrono::milliseconds readTimeout
    )
{
    if (static_cast<uint32_t>(length) > (buffer_size * sizeof(capnp::word)))
    {
        buffer_size = 1 + length / sizeof(capnp::word);
        proto_buffer = kj::heapArray<capnp::word>(buffer_size);
        loggedLengthOfZero = false;
    }

    bytes_read = unixsocket::readFully(sysCallWrapper,
                                       socket_fd.get(),
                                       reinterpret_cast<char*>(proto_buffer.begin()),
                                       length,
                                       readTimeout);
    if (bytes_read < 0)
    {
        std::stringstream errSS;
        errSS << "Aborting: " << common::safer_strerror(errno);
        errMsg = errSS.str();
        return false;
    }
    else if (bytes_read != length)
    {
        errMsg = "Aborting: failed to read entire message";
        return false;
    }

    return true;
}