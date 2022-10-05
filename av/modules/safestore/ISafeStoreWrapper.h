// Copyright 2022, Sophos Limited.  All rights reserved.

/*
 * This is a C++ wrapper around the Core safestore C library.
 * This wrapper interface provides two main benefits, to be able to call the library via C++ methods and to allow
 * unit tests to be written by being able to create a mock of this interface.
 */

#pragma once

#include <optional>
#include <string>

namespace safestore
{
    // Return codes from SafeStore initialisation
    enum class InitReturnCode
    {
        // SafeStore initialised ok
        OK,
        // ...add comments to all  codes here:
        INVALID_ARG,
        UNSUPPORTED_OS,
        UNSUPPORTED_VERSION,
        OUT_OF_MEMORY,
        DB_OPEN_FAILED,
        DB_ERROR,

        // Failed to initialise for unknown reason
        FAILED
    };

    enum class SaveFileReturnCode
    {
        // SafeStore initialised ok
        OK,
        INVALID_ARG,
        INTERNAL_ERROR,
        OUT_OF_MEMORY,
        FILE_OPEN_FAILED,
        FILE_READ_FAILED,
        FILE_WRITE_FAILED,
        MAX_OBJECT_SIZE_EXCEEDED,
        MAX_STORE_SIZE_EXCEEDED,
        DB_ERROR,
        // Failed to save for unknown reason
        FAILED
    };

    enum class ConfigOption
    {
        // Maximum allowed size of SafeStore (database and stored blob files) in bytes, uint64_t
        MAX_SAFESTORE_SIZE,

        // Maximum allowed size of a saved object in bytes, uint64_t
        MAX_OBJECT_SIZE,

        // Maximum allowed number of objects in a saved registry subtree, uint64_t
        MAX_REG_OBJECT_COUNT,

        // If set to true (non-zero): automatically purge SafeStore of the oldest saved objects, bool
        AUTO_PURGE,

        // Maximum allowed number of object stored in SafeStore simultaneously. Must be at least as big as
        // SC_MAX_REG_OBJECT_COUNT, uint64_t
        MAX_STORED_OBJECT_COUNT
    };

    enum class ObjectType
    {
        ANY,
        UNKNOWN = ANY,
        FILE,
        REGKEY,
        REGVALUE,
        LAST = REGVALUE
    };

    enum class ObjectStatus
    {
        ANY,
        UNDEFINED = ANY,
        STORED,
        QUARANTINED,
        RESTORE_FAILED,
        RESTORED_AS,
        RESTORED,
        LAST = RESTORED
    };

    using SafeStoreContext = void*;
    using SafeStoreObjectHandle = void*;
    using SafeStoreSearchHandle = void*;

    // Contains information used to get filtered data from SafeStore.
    struct SafeStoreFilter
    {
        // A combination of one or more SafeStore_FilterField_t flags.
        int activeFields;

        // Threat id the object was saved with.
        std::string threatId;

        // Threat name the object was saved with.
        std::string threatName;

        // Time interval in which objects were saved. UNIX timestamp in seconds.
        int64_t startTime;
        int64_t endTime;

        // Type of the objects to consider.
        ObjectType objectType;

        // Status of the objects to consider.
        ObjectStatus objectStatus;

        // Location of the objects to consider.
        // For a file this is the full path of the directory containing the file.
        std::string objectLocation;

        // Name of the objects to consider.
        // For a file this is the name of the file.
        std::string objectName;
    };

    class SafeStoreObjectHandleHolder
    {
    public:
        virtual ~SafeStoreObjectHandleHolder() = default;
        virtual SafeStoreObjectHandle* get() = 0;
    };

    class SafeStoreSearchHandleHolder
    {
    public:
        virtual ~SafeStoreSearchHandleHolder() = default;
        virtual SafeStoreSearchHandle* get() = 0;
    };

    class ISafeStoreWrapper
    {
    public:
        virtual ~ISafeStoreWrapper() = default;

        /*
         * Initialise SafeStore and setup any resources needed.
         * Returns: InitReturnCode, to indicate if the operation was successful or not.
         */
        virtual InitReturnCode initialise(
            const std::string& dbDirName,
            const std::string& dbName,
            const std::string& password) = 0;

        /*
         * Saves a file to SafeStore.
         * objectHandle is set to point to stored object.
         */
        virtual SaveFileReturnCode saveFile(
            const std::string& directory,
            const std::string& filename,
            const std::string& threatId,
            const std::string& threatName,
            SafeStoreObjectHandleHolder& objectHandle) = 0;

        /*
         * TODO 5675
         */
        virtual std::optional<uint64_t> getConfigIntValue(ConfigOption option) = 0;

        /*
         * TODO 5675
         */
        virtual bool setConfigIntValue(ConfigOption option, uint64_t value) = 0;

        /*
         * TODO 5675
         */
        virtual bool findFirst(
            SafeStoreFilter filter,
            SafeStoreSearchHandleHolder& searchHandle,
            SafeStoreObjectHandleHolder& objectHandle) = 0;
    };

} // namespace safestore
