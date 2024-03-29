// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "EnvironmentInterruption.h"

#include "Common/SystemCallWrapper/ISystemCallWrapper.h"

#include <chrono>
#include <memory>
#include <string>
#include <utility>

namespace unixsocket
{
    ssize_t readLength(int socket_fd, ssize_t maxSize=128*1024);
    void writeLength(int socket_fd, size_t length);
    bool writeLengthAndBuffer(
        Common::SystemCallWrapper::ISystemCallWrapper& systemCallWrapper,
        int socket_fd,
        const std::string& buffer);
    int recv_fd(Common::SystemCallWrapper::ISystemCallWrapper& systemCallWrapper, int socket);
    ssize_t send_fd(int socket, int fd);
    ssize_t readFully(
        const Common::SystemCallWrapper::ISystemCallWrapperSharedPtr&,
        int socket_fd, char* buf, ssize_t bytes, std::chrono::milliseconds timeout);

    bool writeLengthAndBufferAndFd(
        Common::SystemCallWrapper::ISystemCallWrapper& systemCallWrapper,
        int socket_fd,
        const std::string& buffer,
        int fd);

    const int SU_EOF = -2;
    const int SU_ERROR = -1;
    const int SU_ZERO = 0;
} // namespace unixsocket
