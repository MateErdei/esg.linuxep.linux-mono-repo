// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "datatypes/AutoFd.h"

#include <string>

namespace safestore::QuarantineManager
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
         * Initialise the Quarantine Manager so that files can be quarantined.
         */
        virtual void initialise() = 0;

        /*
         * Return the current state of the Quarantine Manager.
         * The implementing class must keep this state updated.
         */
        virtual QuarantineManagerState getState() = 0;

        /*
         * Delete all files within the database directory.
         * This can be called when getState() reports corrupt so that a new database can be initialised.
         */
        virtual bool deleteDatabase() = 0;

        /*
         * Quarantine a file.
         * Add the file to the quarantine database.
         * Delete the file.
         * Store the checksum of the file.
         */
        virtual bool quarantineFile(
            const std::string& filePath,
            const std::string& threatId,
            const std::string& threatName,
            const std::string& sha256,
            datatypes::AutoFd autoFd) = 0;
    };

} // namespace safestore::QuarantineManager
