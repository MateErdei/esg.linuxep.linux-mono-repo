// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include <memory>

namespace Common
{
    class ISignalHandler
    {
        public:
            virtual ~ISignalHandler() = default;

            virtual bool clearSubProcessExitPipe() = 0;
            virtual bool clearTerminationPipe() = 0;
            virtual bool clearUSR1Pipe() = 0;

            virtual int subprocessExitFileDescriptor() = 0;
            virtual int terminationFileDescriptor() = 0;
            virtual int usr1FileDescriptor() = 0;
    };

    using ISignalHandlerSharedPtr = std::shared_ptr<ISignalHandler>;
}
