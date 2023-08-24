// Copyright 2023 Sophos All rights reserved.
#pragma once

#include "Common/SystemCallWrapper/ISystemCallWrapper.h"

namespace unixsocket
{
    class ReadLengthAsync
    {
    public:
        using ISystemCallWrapperSharedPtr = Common::SystemCallWrapper::ISystemCallWrapperSharedPtr;
        explicit ReadLengthAsync(ISystemCallWrapperSharedPtr systemCalls, ssize_t maxSize);
        [[nodiscard]] ssize_t getLength() const;
        int read(int fd);
        bool complete();
        void reset();

    private:
        ISystemCallWrapperSharedPtr systemCalls_;
        ssize_t currentLength_ = 0;
        const ssize_t maxSize_ = 0;
        bool complete_ = false;
        int tooBig();
    };
}
