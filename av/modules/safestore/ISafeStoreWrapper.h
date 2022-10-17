// Copyright 2022, Sophos Limited.  All rights reserved.

/*
 * This is a C++ wrapper around the Core SafeStore C library.
 * This wrapper interface provides two main benefits, to be able to call the library via C++ methods and to allow
 * unit tests to be written by being able to create a mock of this interface.
 */

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace safestore
{
    // Handle the void* typedefs in safestore.h
    // These resources are wrapped up in ObjectHandleHolder, SearchHandleHolder and the SafeStoreWrapperImpl.
    using SafeStoreContext = void*;
    using SafeStoreObjectHandle = void*;
    using SafeStoreSearchHandle = void*;

    // The length in bytes that a threat ID must be
    static constexpr size_t THREAT_ID_LENGTH = 16;

    // Return codes from SafeStore initialisation
    enum class InitReturnCode
    {
        // TODO 5675 document this

        // SafeStore initialised ok
        OK,
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

        // TODO 5675 document this

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
        // TODO 5675 document this

        ANY,
        UNKNOWN = ANY,
        FILE,
        REGKEY,
        REGVALUE,
        LAST = REGVALUE
    };

    enum class ObjectStatus
    {
        // TODO 5675 document this

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

    class SearchResults;
    class ObjectHandleHolder;
    class SearchHandleHolder;

    class ISafeStoreWrapper
    {
    public:
        virtual ~ISafeStoreWrapper() = default;

        /*
         * TODO 5675 Interface docs
         */
        virtual std::unique_ptr<SearchHandleHolder> createSearchHandleHolder() = 0;

        /*
         * TODO 5675 Interface docs
         */
        virtual void releaseSearchHandle(SafeStoreSearchHandle searchHandleHolder) = 0;

        /*
         * TODO 5675 Interface docs
         */
        virtual std::unique_ptr<ObjectHandleHolder> createObjectHandleHolder() = 0;

        /*
         * TODO 5675 Interface docs
         */
        virtual void releaseObjectHandle(SafeStoreObjectHandle objectHandleHolder) = 0;

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
         * TODO 5675 Interface docs
         */
        virtual std::optional<uint64_t> getConfigIntValue(ConfigOption option) = 0;

        /*
         * TODO 5675 Interface docs
         */
        virtual bool setConfigIntValue(ConfigOption option, uint64_t value) = 0;

        /*
         * TODO 5675 Interface docs
         */
        virtual SearchResults find(const SafeStoreFilter& filter) = 0;

        /*
         * TODO 5675 Interface docs
         */
        virtual bool findFirst(
            const SafeStoreFilter& filter,
            SearchHandleHolder& searchHandle,
            ObjectHandleHolder& objectHandle) = 0;

        /*
         * TODO 5675 Interface docs
         */
        virtual bool findNext(SearchHandleHolder& searchHandle, ObjectHandleHolder& objectHandle) = 0;

        /*
         * TODO 5675 Interface docs
         */
        virtual std::string getObjectName(const ObjectHandleHolder& objectHandle) = 0;

        /*
         * TODO 5675 Interface docs
         */
        virtual std::vector<uint8_t> getObjectId(const ObjectHandleHolder& objectHandle) = 0;

        /*
         * TODO 5675 Interface docs
         */
        virtual ObjectType getObjectType(const ObjectHandleHolder& objectHandle) = 0;

        /*
         * TODO 5675 Interface docs
         */
        virtual ObjectStatus getObjectStatus(const ObjectHandleHolder& objectHandle) = 0;

        /*
         * TODO 5675 Interface docs
         */
        virtual std::string getObjectThreatId(const ObjectHandleHolder& objectHandle) = 0;

        /*
         * TODO 5675 Interface docs
         */
        virtual std::string getObjectThreatName(const ObjectHandleHolder& objectHandle) = 0;

        /*
         * TODO 5675 Interface docs
         */
        virtual int64_t getObjectStoreTime(const ObjectHandleHolder& objectHandle) = 0;

        /*
         * TODO 5675 Interface docs
         */
        virtual bool setObjectCustomData(
            ObjectHandleHolder& objectHandle,
            const std::string& dataName,
            const std::vector<uint8_t>& value) = 0;

        /*
         * TODO 5675 Interface docs
         */
        virtual std::vector<uint8_t> getObjectCustomData(
            const ObjectHandleHolder& objectHandle,
            const std::string& dataName) = 0;

        /*
         * TODO 5675 Interface docs
         */
        virtual bool setObjectCustomDataString(
            ObjectHandleHolder& objectHandle,
            const std::string& dataName,
            const std::string& value) = 0;

        /*
         * TODO 5675 Interface docs
         */
        virtual std::string getObjectCustomDataString(
            ObjectHandleHolder& objectHandle,
            const std::string& dataName) = 0;

        // TODO 5675 SafeStore_Result_t SAFESTORE_CALL SafeStore_FinalizeObjectsByThreatId(_In_ SafeStore_t ctx, _In_
        // SafeStore_Id_t* threatId);

        // TODO 5675  _Check_return_ SafeStore_Result_t SAFESTORE_CALL SafeStore_RestoreObjectById(_In_ SafeStore_t ctx,
        // _In_ const SafeStore_Id_t* objectId, _In_opt_z_ const SsPlatChar* location);

        // TODO 5675 _Check_return_ SafeStore_Result_t SAFESTORE_CALL SafeStore_RestoreObjectsByThreatId(_In_
        // SafeStore_t ctx, _In_ const SafeStore_Id_t* threatId);

        // TODO 5675 _Check_return_ SafeStore_Result_t SAFESTORE_CALL SafeStore_DeleteObjectById(_In_ SafeStore_t ctx,
        // _In_ const SafeStore_Id_t* objectId);

        // TODO 5675 _Check_return_ SafeStore_Result_t SAFESTORE_CALL SafeStore_DeleteObjectsByThreatId(_In_ SafeStore_t
        // ctx, _In_ const SafeStore_Id_t* threatId);

        // TODO 5675 SafeStore_Result_t SAFESTORE_CALL SafeStore_ExportFile(_In_ SafeStore_t ctx, _In_ const
        // SafeStore_Id_t* objectId, _Reserved_ const uint8_t* password, _Reserved_ size_t passwordSize, _In_z_ const
        // SsPlatChar* directory, _In_opt_z_ const SsPlatChar* fileName);

        /*
         * TODO 5675 Interface docs
         */
        virtual bool getObjectHandle(const std::string& objectId, std::shared_ptr<ObjectHandleHolder> objectHandle) = 0;

        /*
         * TODO 5675 Interface docs
         */
        virtual bool finaliseObject(ObjectHandleHolder& objectHandle) = 0;
    };

    class ObjectHandleHolder
    {
    public:
        explicit ObjectHandleHolder(ISafeStoreWrapper& safeStoreWrapper) : m_safeStoreWrapper(safeStoreWrapper) {}
        ObjectHandleHolder(const ObjectHandleHolder&) = delete;
        ObjectHandleHolder& operator=(const ObjectHandleHolder&) = delete;

        ~ObjectHandleHolder()
        {
            if (m_handle != nullptr)
            {
                m_safeStoreWrapper.releaseObjectHandle(m_handle);
                m_handle = nullptr;
                //                LOGTRACE("Cleaned up SafeStore object handle");
            }
        }

        SafeStoreObjectHandle getRawHandle() const
        {
            return m_handle;
        }

        SafeStoreObjectHandle* getRawHandlePtr()
        {
            return &m_handle;
        }

        bool operator==(const ObjectHandleHolder& other) const
        {
            //            return m_handle == other.getRawHandle();
            //            return m_safeStoreWrapper.getObjectId(*this) == m_safeStoreWrapper.getObjectId(other);
            auto thisId = m_safeStoreWrapper.getObjectId(*this);
            auto otherId = m_safeStoreWrapper.getObjectId(other);
            return thisId == otherId;
        }

        bool operator!=(const ObjectHandleHolder& other) const
        {
            //            return m_handle == other.getRawHandle();
            //            return m_safeStoreWrapper.getObjectId(*this) == m_safeStoreWrapper.getObjectId(other);
            auto thisId = m_safeStoreWrapper.getObjectId(*this);
            auto otherId = m_safeStoreWrapper.getObjectId(other);
            return thisId != otherId;
        }

    private:
        SafeStoreObjectHandle m_handle = nullptr;
        ISafeStoreWrapper& m_safeStoreWrapper;
    };

    class SearchHandleHolder
    {
    public:
        explicit SearchHandleHolder(ISafeStoreWrapper& safeStoreWrapper) : m_safeStore(safeStoreWrapper) {}
        SearchHandleHolder(const SearchHandleHolder&) = delete;
        SearchHandleHolder& operator=(const SearchHandleHolder&) = delete;

        ~SearchHandleHolder()
        {
            if (m_handle != nullptr)
            {
                m_safeStore.releaseSearchHandle(m_handle);
                m_handle = nullptr;
                //                LOGTRACE("Cleaned up SafeStore search handle");
            }
        }

        SafeStoreSearchHandle* getRawHandle()
        {
            return &m_handle;
        }

    private:
        SafeStoreSearchHandle m_handle = nullptr;
        ISafeStoreWrapper& m_safeStore;
    };

    class SearchResults
    {
    public:
        SearchResults(ISafeStoreWrapper& safeStore, const SafeStoreFilter& filter) :
            m_safeStore(safeStore), m_filter(filter)
        {
        }

        struct Iterator
        {
            using iterator_category = std::forward_iterator_tag;

            Iterator(ISafeStoreWrapper& m_safeStore) :
                m_safeStore(m_safeStore),
                m_searchHolder(m_safeStore.createSearchHandleHolder()),
                m_objectHolder(m_safeStore.createObjectHandleHolder())
            {
                // Used for end(), don't set anything, the m_objectHolder should be null.
            }

            Iterator(ISafeStoreWrapper& m_safeStore, const SafeStoreFilter& filter) :
                m_safeStore(m_safeStore),
                m_searchHolder(m_safeStore.createSearchHandleHolder()),
                m_objectHolder(m_safeStore.createObjectHandleHolder())
            {
                m_safeStore.findFirst(filter, *m_searchHolder, *m_objectHolder);
            }

            ObjectHandleHolder& operator*() const
            {
                return *m_objectHolder;
            }

            ObjectHandleHolder* operator->()
            {
                return &(*m_objectHolder);
            }

            // Pre-increment ++i
            Iterator& operator++()
            {
                m_safeStore.findNext(*m_searchHolder, *m_objectHolder);
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
            };

            friend bool operator!=(const Iterator& a, const Iterator& b)
            {
                return *a.m_objectHolder != *b.m_objectHolder;
            };

        private:
            ISafeStoreWrapper& m_safeStore;
            std::shared_ptr<SearchHandleHolder> m_searchHolder;
            std::shared_ptr<ObjectHandleHolder> m_objectHolder;
        };

        Iterator begin()
        {
            return Iterator(m_safeStore, m_filter);
        }

        Iterator end()
        {
            return Iterator(m_safeStore);
        }

        ISafeStoreWrapper& m_safeStore;
        SafeStoreFilter m_filter;
    };

} // namespace safestore
