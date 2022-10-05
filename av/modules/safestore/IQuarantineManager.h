// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

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
        virtual void initialise() = 0;
        virtual QuarantineManagerState getState() = 0;
        virtual bool deleteDatabase() = 0;
        virtual bool quarantineFile(
            const std::string& directory,
            const std::string& filename,
            const std::string& threatId,
            const std::string& threatName) = 0;
    };

} // namespace safestore
