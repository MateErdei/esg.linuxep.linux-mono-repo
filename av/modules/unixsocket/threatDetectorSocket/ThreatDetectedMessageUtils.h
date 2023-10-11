// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "datatypes/AutoFd.h"

#include "Common/SystemCallWrapper/ISystemCallWrapper.h"

#include <capnp/message.h>

namespace unixsocket
{
    bool isReceivedFdFile(
        std::shared_ptr<Common::SystemCallWrapper::ISystemCallWrapper> sysCallWrapper,
        datatypes::AutoFd& file_fd,
        std::string& errMsg);
    bool isReceivedFileOpen(
        std::shared_ptr<Common::SystemCallWrapper::ISystemCallWrapper> sysCallWrapper,
        datatypes::AutoFd& file_fd,
        std::string& errMsg);
    bool readCapnProtoMsg(
        const std::shared_ptr<Common::SystemCallWrapper::ISystemCallWrapper>& sysCallWrapper,
        ssize_t length,
        uint32_t& buffer_size,
        kj::Array<capnp::word>& proto_buffer,
        datatypes::AutoFd& socket_fd,
        ssize_t& bytes_read,
        bool& loggedLengthOfZero,
        std::string& errMsg);
}