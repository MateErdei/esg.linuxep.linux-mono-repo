// Copyright 2023 Sophos All rights reserved.
#pragma once

#include "Common/SystemCallWrapper/ISystemCallWrapper.h"

#include <capnp/common.h>
#include <kj/array.h>

namespace unixsocket
{
    class ReadBufferAsync
    {
    public:
        using ISystemCallWrapperSharedPtr = Common::SystemCallWrapper::ISystemCallWrapperSharedPtr;
        explicit ReadBufferAsync(ISystemCallWrapperSharedPtr systemCalls, size_t defaultSize);

        /**
         *
         * @param size
         * @return True if a new buffer was allocated
         */
        bool setLength(size_t size);

        int read(int fd);

        [[nodiscard]] bool complete() const
        {
            return position_ == expectedSize_;
        }

        kj::Array<capnp::word>& getBuffer()
        {
            return proto_buffer_;
        }

    private:
        ISystemCallWrapperSharedPtr systemCalls_;
        size_t buffer_size_ = 0;
        kj::Array<capnp::word> proto_buffer_;
        size_t expectedSize_ = 0;
        size_t position_ = 0;
    };
}
