// Copyright 2022, Sophos Limited. All rights reserved.

/*
 * This is a C++ wrapper around the Core SafeStore C library.
 * This wrapper interface provides two main benefits, to be able to call the library via C++ methods and to allow
 * unit tests to be written by being able to create a mock of this interface.
 *
 * Notes
 * SafeStore_FinalizeObjectsByThreatId has not been implemented for any platform by SafeStore library team.
 */

#pragma once

#include <cassert>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace safestore::SafeStoreWrapper
{
    // Handle the void* typedefs in safestore.h
    // These resources are wrapped up in ObjectHandleHolder and the SafeStoreWrapperImpl.
    using SafeStoreContext = void*;
    using SafeStoreObjectHandle = void*;
    // ObjectId and ThreatId are UUID strings
    using ObjectId = std::string;
    using ThreatId = std::string;

    static constexpr size_t MAX_OBJECT_NAME_LENGTH = 300;
    static constexpr size_t MAX_OBJECT_PATH_LENGTH = 5000;
    static constexpr size_t MAX_OBJECT_THREAT_NAME_LENGTH = 200;
    static constexpr size_t MAX_CUSTOM_DATA_SIZE = 5000; // bytes

    // Return codes from SafeStore initialisation
    enum class InitReturnCode
    {
        OK,
        INVALID_ARG,
        UNSUPPORTED_OS,
        UNSUPPORTED_VERSION,
        OUT_OF_MEMORY,
        DB_OPEN_FAILED,
        DB_ERROR,
        FAILED // Failed to initialise for unknown reason
    };

    enum class SaveFileReturnCode
    {
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
        FAILED // Failed to save for unknown reason
    };

    static const std::map<SaveFileReturnCode, std::string> GL_SAVE_FILE_RETURN_CODES
        {
            { SaveFileReturnCode::OK, "OK" },
            { SaveFileReturnCode::INVALID_ARG, "InvalidArg" },
            { SaveFileReturnCode::INTERNAL_ERROR , "InternalError" },
            { SaveFileReturnCode::OUT_OF_MEMORY, "OutOfMemory" },
            { SaveFileReturnCode::FILE_OPEN_FAILED, "FileOpenFailed" },
            { SaveFileReturnCode::FILE_READ_FAILED, "FileReadFailed" },
            { SaveFileReturnCode::FILE_WRITE_FAILED, "FileWriteFailed" },
            { SaveFileReturnCode::MAX_OBJECT_SIZE_EXCEEDED, "MaxObjectSizeExceeded" },
            { SaveFileReturnCode::MAX_STORE_SIZE_EXCEEDED, "MaxStoreSizeExceeded" },
            { SaveFileReturnCode::DB_ERROR, "DatabaseError" },
            { SaveFileReturnCode::FAILED, "Unknown" },
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

    static const std::map<ConfigOption, std::string> GL_OPTIONS_MAP
    {
        { ConfigOption::MAX_SAFESTORE_SIZE, "MaxSafeStoreSize" },
        { ConfigOption::MAX_OBJECT_SIZE, "MaxObjectSize" },
        { ConfigOption::MAX_REG_OBJECT_COUNT , "MaxRegObjectCount" },
        { ConfigOption::AUTO_PURGE, "AutoPurge" },
        { ConfigOption::MAX_STORED_OBJECT_COUNT, "MaxStoredObjectCount" },
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

    // Contains information used to get filtered data from SafeStore.
    // Options set to nullopt will not affect the results
    struct SafeStoreFilter
    {
        // Threat id the object was saved with.
        std::optional<std::string> threatId;

        // Threat name the object was saved with.
        std::optional<std::string> threatName;

        // Time interval in which objects were saved. UNIX timestamp in seconds.
        // Both do not have to be specified if only one is relevant.
        std::optional<int64_t> startTime;
        std::optional<int64_t> endTime;

        // Type of the objects to consider.
        std::optional<ObjectType> objectType;

        // Status of the objects to consider.
        std::optional<ObjectStatus> objectStatus;

        // Location of the objects to consider.
        // For a file this is the full path of the directory containing the file.
        std::optional<std::string> objectLocation;

        // Name of the objects to consider.
        // For a file this is the name of the file.
        std::optional<std::string> objectName;

        bool operator==(const SafeStoreFilter& other) const
        {
            return threatId == other.threatId && threatName == other.threatName && startTime == other.startTime &&
                   endTime == other.endTime && objectType == other.objectType && objectStatus == other.objectStatus &&
                   objectLocation == other.objectLocation && objectName == other.objectName;
        }
    };

    class ISafeStoreHolder
    {
    public:
        virtual ~ISafeStoreHolder() = default;
        virtual InitReturnCode init(
            const std::string& dbDirName,
            const std::string& dbName,
            const std::string& password) = 0;
        virtual void deinit() = 0;
        virtual SafeStoreContext getHandle() = 0;
    };

    class ISafeStoreReleaseMethods
    {
    public:
        virtual ~ISafeStoreReleaseMethods() = default;

        /*
         * Utility function to clean up an object handle
         */
        virtual void releaseObjectHandle(SafeStoreObjectHandle objectHandleHolder) = 0;
    };

    class ISafeStoreGetIdMethods
    {
    public:
        virtual ~ISafeStoreGetIdMethods() = default;

        /*
         * Return the ID of an object. The ID of an object is generated by SafeStore and consists of 16 random bytes.
         */
        virtual ObjectId getObjectId(SafeStoreObjectHandle objectHandle) = 0;
    };

    class ObjectHandleHolder
    {
    public:
        ObjectHandleHolder(
            std::shared_ptr<ISafeStoreGetIdMethods> getIdMethods,
            std::shared_ptr<ISafeStoreReleaseMethods> releaseMethods) :
            m_getIdMethods(std::move(getIdMethods)), m_safeStoreReleaseMethods(std::move(releaseMethods))
        {
            assert(m_getIdMethods);
            assert(m_safeStoreReleaseMethods);
        }
        ObjectHandleHolder(const ObjectHandleHolder&) = delete;
        ObjectHandleHolder& operator=(const ObjectHandleHolder&) = delete;

        ObjectHandleHolder(ObjectHandleHolder&& other) noexcept :
            m_handle(other.m_handle),
            m_getIdMethods(std::move(other.m_getIdMethods)),
            m_safeStoreReleaseMethods(std::move(other.m_safeStoreReleaseMethods))
        {
            other.m_handle = nullptr;
        }
        ObjectHandleHolder& operator=(ObjectHandleHolder&& other) noexcept
        {
            std::swap(m_handle, other.m_handle);
            std::swap(m_getIdMethods, other.m_getIdMethods);
            std::swap(m_safeStoreReleaseMethods, other.m_safeStoreReleaseMethods);
            return *this;
        }

        ~ObjectHandleHolder()
        {
            if (m_safeStoreReleaseMethods)
            {
                m_safeStoreReleaseMethods->releaseObjectHandle(m_handle);
            }
            else
            {
                assert(m_handle == nullptr);
            }
        }

        [[nodiscard]] SafeStoreObjectHandle getRawHandle() const
        {
            return m_handle;
        }

        SafeStoreObjectHandle* getRawHandlePtr()
        {
            return &m_handle;
        }

        bool operator==(const ObjectHandleHolder& other) const
        {
            return m_getIdMethods->getObjectId(m_handle) == m_getIdMethods->getObjectId(other.getRawHandle());
        }

        bool operator!=(const ObjectHandleHolder& other) const
        {
            return !(*this == other);
        }

    private:
        SafeStoreObjectHandle m_handle = nullptr;
        std::shared_ptr<ISafeStoreGetIdMethods> m_getIdMethods = nullptr;
        std::shared_ptr<ISafeStoreReleaseMethods> m_safeStoreReleaseMethods = nullptr;
    };

    class ISafeStoreWrapper
    {
    public:
        virtual ~ISafeStoreWrapper() = default;

        /*
         * Utility function to generate a SafeStore object handle, the handle needs a reference to this interface to
         * clean up on destruction.
         */
        virtual std::unique_ptr<ObjectHandleHolder> createObjectHandleHolder() = 0;

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
            const ThreatId& threatId,
            const std::string& threatName,
            ObjectHandleHolder& objectHandle) = 0;

        /*
         * Get a SafeStore config option. If it is not available or the call fails then the value  is not set.
         */
        virtual std::optional<uint64_t> getConfigIntValue(ConfigOption option) = 0;

        /*
         * Set a SafeStore config option. If setting the value was successful then this returns true, false otherwise.
         */
        virtual bool setConfigIntValue(ConfigOption option, uint64_t value) = 0;

        /*
         * Find objects in the SafeStore database based on a filter, returns an iterator to access the result objects.
         */
        virtual std::vector<ObjectHandleHolder> find(const SafeStoreFilter& filter) = 0;

        /*
         * Return the name of an object in the SafeStore database.
         * The object handle that gets passed in must be holding a SafeStore object.
         * If the raw handle in the object holder is null or the call into SafeStore fails
         * an empty string is returned.
         */
        virtual std::string getObjectName(const ObjectHandleHolder& objectHandle) = 0;

        /*
         * Return the location/path of an object on disk before it was saved in the SafeStore database.
         * The object handle that gets passed in must be holding a SafeStore object.
         * If the raw handle in the object holder is null or the call into SafeStore fails
         * an empty string is returned.
         */
        virtual std::string getObjectLocation(const ObjectHandleHolder& objectHandle) = 0;

        /*
         * Return the ID of an object. The ID of an object is generated by SafeStore and consists of 16 random bytes.
         */
        virtual ObjectId getObjectId(const ObjectHandleHolder& objectHandle) = 0;

        /*
         * Get the type of an object in the SafeStore database.
         * The object handle that gets passed in must be holding a SafeStore object.
         * If the raw handle in the object holder is null or the call into SafeStore fails
         * ObjectType::UNKNOWN is returned.
         */
        virtual ObjectType getObjectType(const ObjectHandleHolder& objectHandle) = 0;

        /*
         * Get the status of an object in the SafeStore database.
         * The object handle that gets passed in must be holding a SafeStore object.
         * If the raw handle in the object holder is null or the call into SafeStore fails
         * ObjectStatus::UNDEFINED is returned.
         */
        virtual ObjectStatus getObjectStatus(const ObjectHandleHolder& objectHandle) = 0;

        /*
         * Get the Threat ID of an object in the SafeStore database.
         * This is the value that is passed in during quarantine.
         * The object handle that gets passed in must be holding a SafeStore object.
         * If the raw handle in the object holder is null or the call into SafeStore fails
         * then an empty string is returned.
         */
        virtual ThreatId getObjectThreatId(const ObjectHandleHolder& objectHandle) = 0;

        /*
         * Get the Threat Name of an object in the SafeStore database.
         * This is the value that is passed in during quarantine.
         * The object handle that gets passed in must be holding a SafeStore object.
         * If the raw handle in the object holder is null or the call into SafeStore fails
         * then an empty string is returned.
         */
        virtual std::string getObjectThreatName(const ObjectHandleHolder& objectHandle) = 0;

        /*
         * Get the time that an object was stored in the SafeStore database.
         * This value is a UNIX timestamp (seconds from epoch)
         * The object handle that gets passed in must be holding a SafeStore object.
         * If the raw handle in the object holder is null or the call into SafeStore fails
         * then 0 is returned.
         */
        virtual int64_t getObjectStoreTime(const ObjectHandleHolder& objectHandle) = 0;

        /*
         * Store custom data against an object in the SafeStore database.
         * The object handle that gets passed in must be holding a SafeStore object.
         * If the raw handle in the object holder is null or the call into SafeStore fails
         * then this will return false.
         * dataName is the name to tag the data with and this is what will need to be used to retrieve that data.
         * value is a vector of bytes to be stored.
         */
        virtual bool setObjectCustomData(
            ObjectHandleHolder& objectHandle,
            const std::string& dataName,
            const std::vector<uint8_t>& value) = 0;

        /*
         * Retrieve custom data that has been store against an object in the SafeStore database.
         * The object handle that gets passed in must be holding a SafeStore object.
         * If the raw handle in the object holder is null or the call into SafeStore fails
         * then this will return an empty vector of bytes.
         * dataName is the name that the data was tagged with when it was stored by a call to setObjectCustomData
         */
        virtual std::vector<uint8_t> getObjectCustomData(
            const ObjectHandleHolder& objectHandle,
            const std::string& dataName) = 0;

        /*
         * Store string data against an object in the SafeStore database.
         * This is a wrapper around setObjectCustomData that will allow easy string
         * storage and has a respective get method: getObjectCustomDataString.
         * The object handle that gets passed in must be holding a SafeStore object.
         * If the raw handle in the object holder is null or the call into SafeStore fails
         * then this will return false.
         * dataName is the name to tag the data with and this is what will need to be used to retrieve that data.
         * value is your custom string to be stored.
         */
        virtual bool setObjectCustomDataString(
            ObjectHandleHolder& objectHandle,
            const std::string& dataName,
            const std::string& value) = 0;

        /*
         * Retrieve string data that has been store against an object in the SafeStore database.
         * The object handle that gets passed in must be holding a SafeStore object.
         * If the raw handle in the object holder is null or the call into SafeStore fails
         * then this will return an empty string.
         * dataName is the name that the string was tagged with when it was stored by a
         * call to setObjectCustomDataString.
         */
        virtual std::string getObjectCustomDataString(
            ObjectHandleHolder& objectHandle,
            const std::string& dataName) = 0;

        /*
         * Restore a file, specified by its object ID, to its original location.
         * Returns true on success and false on failure.
         */
        virtual bool restoreObjectById(const ObjectId& objectId) = 0;

        /*
         * Restore a file, specified by its object ID, to a custom location.
         * The dirPath argument specifies the parent directory the file is to be restored to.
         * If the dirPath is an empty string then the original path of the file is used.
         * Returns true on success and false on failure.
         */
        virtual bool restoreObjectByIdToLocation(const ObjectId& objectId, const std::string& dirPath) = 0;

        /*
         * Restore all files that have the same threat ID to their original locations.
         * Returns true on success and false on failure.
         */
        virtual bool restoreObjectsByThreatId(const ThreatId& threatId) = 0;

        /*
         * Remove a file, specified by its object ID, from the SafeStore database.
         * Returns true on success and false on failure.
         */
        virtual bool deleteObjectById(const ObjectId& objectId) = 0;

        /*
         * Remove all files with the specified threat ID, from the SafeStore database.
         * Returns true on success and false on failure.
         */
        virtual bool deleteObjectsByThreatId(const ThreatId& threatId) = 0;

        /*
         * Retrieve an objecthandle from the SafeStore database to perform operations using that object, such as
         * getting a Threat ID or name of the object.
         * The object handle that gets passed in must be holding a SafeStore object.
         * If the raw handle in the object holder is null or the call into SafeStore fails
         * then this will return false.
         * objectId is the ID of the object that was generated when a file was saved to SafeStore.
         */
        virtual bool getObjectHandle(const ObjectId& objectId, std::shared_ptr<ObjectHandleHolder> objectHandle) = 0;

        /*
         * Mark an object as finalised to indicate that all needed operations have been performed and that file.
         * The object handle that gets passed in must be holding a SafeStore object.
         * If the raw handle in the object holder is null or the call into SafeStore fails
         * then this will return false. If the finalise call is successful then true is returned.
         */
        virtual bool finaliseObject(ObjectHandleHolder& objectHandle) = 0;
    };

} // namespace safestore::SafeStoreWrapper
