// Copyright 2022, Sophos Limited.  All rights reserved.

/*
 * This is a C++ wrapper around the Core SafeStore C library.
 * This wrapper interface provides two main benefits, to be able to call the library via C++ methods and to allow
 * unit tests to be written by being able to create a mock of this interface.
 *
 * Notes
 * SafeStore_FinalizeObjectsByThreatId has not been implemented for any platform by SafeStore library team.
 */

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace safestore::SafeStoreWrapper
{
    // Handle the void* typedefs in safestore.h
    // These resources are wrapped up in ObjectHandleHolder, SearchHandleHolder and the SafeStoreWrapperImpl.
    using SafeStoreContext = void*;
    using SafeStoreObjectHandle = void*;
    using SafeStoreSearchHandle = void*;
    using ObjectIdType = std::vector<uint8_t>;

    // The length in bytes that a threat ID must be
    static constexpr size_t THREAT_ID_LENGTH = 16;
    static constexpr int MAX_OBJECT_NAME_LENGTH = 200;
    static constexpr int MAX_OBJECT_THREAT_NAME_LENGTH = 200;
    static constexpr int MAX_CUSTOM_DATA_SIZE = 5000; // bytes

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

    enum class FilterField
    {
        THREAT_ID = 0x0001,
        THREAT_NAME = 0x0002,
        START_TIME = 0x0004,
        END_TIME = 0x0008,
        OBJECT_TYPE = 0x0010,
        OBJECT_STATUS = 0x0020,
        OBJECT_LOCATION = 0x0040,
        OBJECT_NAME = 0x0080,
    };

    // Contains information used to get filtered data from SafeStore.
    struct SafeStoreFilter
    {
        // A combination of one or more SafeStore_FilterField_t flags.
        std::vector<FilterField> activeFields;

        // Threat id the object was saved with.
        std::string threatId;

        // Threat name the object was saved with.
        std::string threatName;

        // Time interval in which objects were saved. UNIX timestamp in seconds.
        int64_t startTime;
        int64_t endTime;

        // Type of the objects to consider.
        ObjectType objectType = ObjectType::ANY;

        // Status of the objects to consider.
        ObjectStatus objectStatus = ObjectStatus::ANY;

        // Location of the objects to consider.
        // For a file this is the full path of the directory containing the file.
        std::string objectLocation;

        // Name of the objects to consider.
        // For a file this is the name of the file.
        std::string objectName;
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

        /*
         * Utility function to clean up a search handle
         */
        virtual void releaseSearchHandle(SafeStoreSearchHandle searchHandleHolder) = 0;
    };

    class ISafeStoreGetIdMethods
    {
    public:
        virtual ~ISafeStoreGetIdMethods() = default;

        /*
         * Return the ID of an object. The ID of an object is generated by SafeStore and consists of 16 random bytes.
         */
        virtual ObjectIdType getObjectId(SafeStoreObjectHandle objectHandle) = 0;
    };

    class ObjectHandleHolder
    {
    public:
        ObjectHandleHolder(
            std::shared_ptr<ISafeStoreGetIdMethods> getIdMethods,
            std::shared_ptr<ISafeStoreReleaseMethods> releaseMethods) :
            m_getIdMethods(std::move(getIdMethods)), m_safeStoreReleaseMethods(std::move(releaseMethods))
        {
        }
        ObjectHandleHolder(const ObjectHandleHolder&) = delete;
        ObjectHandleHolder& operator=(const ObjectHandleHolder&) = delete;

        ~ObjectHandleHolder()
        {
            m_safeStoreReleaseMethods->releaseObjectHandle(m_handle);
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
        std::shared_ptr<ISafeStoreGetIdMethods> m_getIdMethods;
        std::shared_ptr<ISafeStoreReleaseMethods> m_safeStoreReleaseMethods;
    };

    class SearchHandleHolder
    {
    public:
        explicit SearchHandleHolder(std::shared_ptr<ISafeStoreReleaseMethods> releaseMethods) :
            m_safeStoreReleaseMethods(std::move(releaseMethods))
        {
        }
        SearchHandleHolder(const SearchHandleHolder&) = delete;
        SearchHandleHolder& operator=(const SearchHandleHolder&) = delete;

        ~SearchHandleHolder()
        {
            m_safeStoreReleaseMethods->releaseSearchHandle(m_handle);
        }

        SafeStoreSearchHandle* getRawHandlePtr()
        {
            return &m_handle;
        }

    private:
        SafeStoreSearchHandle m_handle = nullptr;
        std::shared_ptr<ISafeStoreReleaseMethods> m_safeStoreReleaseMethods;
    };

    class ISafeStoreSearchMethods
    {
    public:
        virtual ~ISafeStoreSearchMethods() = default;

        /*
         * Find the first object in the SafeStore database that matches a filter.
         * The search handle can then be used in findNext.
         * The object handle will be set the result of the search.
         * If no object is found then the raw handle within the holder is set to null by SafeStore.
         */
        virtual bool findFirst(
            const SafeStoreFilter& filter,
            SearchHandleHolder& searchHandle,
            ObjectHandleHolder& objectHandle) = 0;

        /*
         * Find the next object that matches a search, here pass in the search handle used from a call to findFirst.
         * The object handle will be set the result of the search.
         * If no object is found then the raw handle within the holder is set to null by SafeStore.
         */
        virtual bool findNext(SearchHandleHolder& searchHandle, ObjectHandleHolder& objectHandle) = 0;
    };

    class SearchResults
    {
    public:
        SearchResults(
            std::shared_ptr<ISafeStoreReleaseMethods> releaseMethods,
            std::shared_ptr<ISafeStoreSearchMethods> searchMethods,
            std::shared_ptr<ISafeStoreGetIdMethods> getIdMethods,
            SafeStoreFilter filter) :
            m_releaseMethods(std::move(releaseMethods)),
            m_searchMethods(std::move(searchMethods)),
            m_getIdMethods(std::move(getIdMethods)),
            m_filter(std::move(filter))
        {
        }

        struct Iterator
        {
            using iterator_category = std::forward_iterator_tag;

            explicit Iterator(
                std::shared_ptr<ISafeStoreReleaseMethods> releaseMethods,
                std::shared_ptr<ISafeStoreSearchMethods> searchMethods,
                const std::shared_ptr<ISafeStoreGetIdMethods>& getIdMethods) :
                m_releaseMethods(std::move(releaseMethods)),
                m_searchMethods(std::move(searchMethods)),
                m_objectHolder(std::make_shared<ObjectHandleHolder>(getIdMethods, m_releaseMethods)),
                m_searchHolder(std::make_shared<SearchHandleHolder>(m_releaseMethods))
            {
                // Used for end(), don't set anything, the m_objectHolder should be null.
            }

            Iterator(
                std::shared_ptr<ISafeStoreReleaseMethods> releaseMethods,
                std::shared_ptr<ISafeStoreSearchMethods> searchMethods,
                const std::shared_ptr<ISafeStoreGetIdMethods>& getIdMethods,
                const SafeStoreFilter& filter) :
                m_releaseMethods(std::move(releaseMethods)),
                m_searchMethods(std::move(searchMethods)),
                m_objectHolder(std::make_shared<ObjectHandleHolder>(getIdMethods, m_releaseMethods)),
                m_searchHolder(std::make_shared<SearchHandleHolder>(m_releaseMethods))
            {
                m_searchMethods->findFirst(filter, *m_searchHolder, *m_objectHolder);
            }

            ObjectHandleHolder& operator*() const
            {
                return *m_objectHolder;
            }

            ObjectHandleHolder* operator->() const
            {
                return m_objectHolder.get();
            }

            // Pre-increment ++i
            Iterator& operator++()
            {
                m_searchMethods->findNext(*m_searchHolder, *m_objectHolder);
                return *this;
            }

            // Post-increment i++
            Iterator operator++(int)
            {
                Iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            friend bool operator==(const Iterator& a, const Iterator& b)
            {
                return *a.m_objectHolder == *b.m_objectHolder;
            }

            friend bool operator!=(const Iterator& a, const Iterator& b)
            {
                return !(a == b);
            }

        private:
            std::shared_ptr<ISafeStoreReleaseMethods> m_releaseMethods;
            std::shared_ptr<ISafeStoreSearchMethods> m_searchMethods;
            std::shared_ptr<ObjectHandleHolder> m_objectHolder;
            std::shared_ptr<SearchHandleHolder> m_searchHolder;
        };

        [[nodiscard]] Iterator begin() const
        {
            return { m_releaseMethods, m_searchMethods, m_getIdMethods, m_filter };
        }

        [[nodiscard]] Iterator end() const
        {
            return Iterator(m_releaseMethods, m_searchMethods, m_getIdMethods);
        }

        std::shared_ptr<ISafeStoreReleaseMethods> m_releaseMethods;
        std::shared_ptr<ISafeStoreSearchMethods> m_searchMethods;
        std::shared_ptr<ISafeStoreGetIdMethods> m_getIdMethods;
        SafeStoreFilter m_filter;
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
         * Utility function to generate a SafeStore search handle, the handle needs a reference to this interface to
         * clean up on destruction.
         */
        virtual std::unique_ptr<SearchHandleHolder> createSearchHandleHolder() = 0;

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
         * The filter must have options set and the active fields member of the filter
         * must match the search criteria that have been set on the filter.
         */
        virtual SearchResults find(const SafeStoreFilter& filter) = 0;

        /*
         * Return the name of an object in the SafeStore database.
         * The object handle that gets passed in must be holding a SafeStore object.
         * If the raw handle in the object holder is null or the call into SafeStore fails
         * an empty string is returned.
         */
        virtual std::string getObjectName(const ObjectHandleHolder& objectHandle) = 0;

        /*
         * Return the ID of an object. The ID of an object is generated by SafeStore and consists of 16 random bytes.
         */
        virtual ObjectIdType getObjectId(const ObjectHandleHolder& objectHandle) = 0;

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
        virtual std::string getObjectThreatId(const ObjectHandleHolder& objectHandle) = 0;

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

        // TODO LINUXDAR-5734  _Check_return_ SafeStore_Result_t SAFESTORE_CALL SafeStore_RestoreObjectById(_In_
        // SafeStore_t ctx, _In_ const SafeStore_Id_t* objectId, _In_opt_z_ const SsPlatChar* location);

        // TODO LINUXDAR-5734 _Check_return_ SafeStore_Result_t SAFESTORE_CALL SafeStore_RestoreObjectsByThreatId(_In_
        // SafeStore_t ctx, _In_ const SafeStore_Id_t* threatId);

        // TODO LINUXDAR-5734 _Check_return_ SafeStore_Result_t SAFESTORE_CALL SafeStore_DeleteObjectById(_In_
        // SafeStore_t ctx, _In_ const SafeStore_Id_t* objectId);

        // TODO LINUXDAR-5734 _Check_return_ SafeStore_Result_t SAFESTORE_CALL SafeStore_DeleteObjectsByThreatId(_In_
        // SafeStore_t ctx, _In_ const SafeStore_Id_t* threatId);

        // TODO LINUXDAR-5734 SafeStore_Result_t SAFESTORE_CALL SafeStore_ExportFile(_In_ SafeStore_t ctx, _In_ const
        // SafeStore_Id_t* objectId, _Reserved_ const uint8_t* password, _Reserved_ size_t passwordSize, _In_z_ const
        // SsPlatChar* directory, _In_opt_z_ const SsPlatChar* fileName);

        /*
         * Retrieve an objecthandle from the safestore database to perform operations using that object, such as
         * getting a Threat ID or name of the object.
         * The object handle that gets passed in must be holding a SafeStore object.
         * If the raw handle in the object holder is null or the call into SafeStore fails
         * then this will return false.
         * objectId is the ID of the object that was generated when a file was saved to SafeStore.
         */
        virtual bool getObjectHandle(const ObjectIdType& objectId, std::shared_ptr<ObjectHandleHolder> objectHandle) = 0;

        /*
         * Mark an object as finalised to indicate that all needed operations have been performed and that file.
         * The object handle that gets passed in must be holding a SafeStore object.
         * If the raw handle in the object holder is null or the call into SafeStore fails
         * then this will return false. If the finalise call is successful then true is returned.
         */
        virtual bool finaliseObject(ObjectHandleHolder& objectHandle) = 0;
    };

} // namespace safestore::SafeStoreWrapper
