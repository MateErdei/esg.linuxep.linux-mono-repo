// Copyright 2022 Sophos Limited. All rights reserved.

#pragma once

#include "common/CentralEnums.h"
#include "datatypes/AutoFd.h"
#include "datatypes/ISystemCallWrapper.h"
#include "safestore/SafeStoreWrapper/ISafeStoreWrapper.h"
#include "scan_messages/QuarantineResponse.h"

#include <string>

namespace safestore::QuarantineManager
{
    using FdsObjectIdsPair = std::pair<datatypes::AutoFd, SafeStoreWrapper::ObjectId>;
    enum class QuarantineManagerState
    {
        // QM is ok and ready to use
        INITIALISED,

        // QM is not initialised and cannot be used
        UNINITIALISED,

        // The DB is corrupt (this is triggered when we keep getting DB errors)
        CORRUPT,

        // The QM is starting up, guarenteed to change
        STARTUP
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
         * Used to set the state of the Quarantine Manager
         */
        virtual void setState(const safestore::QuarantineManager::QuarantineManagerState& newState) = 0;

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
        virtual common::CentralEnums::QuarantineResult quarantineFile(
            const std::string& filePath,
            const std::string& threatId,
            const std::string& threatName,
            const std::string& sha256,
            datatypes::AutoFd autoFd) = 0;

        /**
         * Unpack detections.
         */
        virtual std::vector<FdsObjectIdsPair> extractQuarantinedFiles(
            datatypes::ISystemCallWrapper& sysCallWrapper) = 0;

        /**
         * Run the avscanner on unpacked detections and return vector of objectsIds that are clean.
         */
        virtual std::vector<SafeStoreWrapper::ObjectId> scanExtractedFilesForRestoreList(
            std::vector<FdsObjectIdsPair> files) = 0;

        /**
         * Unpack all detections from database and run the avscanner on them.
         */
        virtual void rescanDatabase() = 0;
    };

} // namespace safestore::QuarantineManager
