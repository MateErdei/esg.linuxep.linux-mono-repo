// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "datatypes/AutoFd.h"

#include <string>

namespace safestore
{
    enum class QuarantineManagerState
    {
        // QM is ok and ready to use
        INITIALISED,

        // QM is not initialised and cannot be used
        UNINITIALISED,

        // The DB is corrupt (this is triggered when we keep getting DB errors)
        CORRUPT
    };

    class IQuarantineManager
    {
    public:
        virtual ~IQuarantineManager() = default;

        /*
         * TODO 5675 Interface docs
         */
        virtual void initialise() = 0;

        /*
         * TODO 5675 Interface docs
         */
        virtual QuarantineManagerState getState() = 0;

        /*
         * TODO 5675 Interface docs
         */
        virtual bool deleteDatabase() = 0;

        /*
         * TODO 5675 Interface docs
         */
        virtual bool quarantineFile(
            const std::string& filePath,
            const std::string& threatId,
            const std::string& threatName,
            const std::string& sha256,
            datatypes::AutoFd autoFd) = 0;
    };

} // namespace safestore
