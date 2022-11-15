// Copyright 2022, Sophos Limited. All rights reserved.

#pragma once

#include "ISafeStoreWrapper.h"

// Ensure this is included first - On Ubuntu 20.04 stat gives an error if not included before safestore.h
/*
In file included from /usr/include/x86_64-linux-gnu/bits/statx.h:31,
from /usr/include/x86_64-linux-gnu/sys/stat.h:446,
from /home/pair/mav/modules/safestore/SafeStoreWrapper/SafeStoreWrapperImpl.h:16,
from /home/pair/mav/modules/safestore/Main.cpp:10:
/usr/include/linux/stat.h:59:9: error: declaration does not declare anything [-fpermissive]
59 |         __s32   __reserved;
   |         ^~~~~
 */
// clang-format off
#include <sys/stat.h>

extern "C"
{
#include <safestore.h>
}
// clang-format on

#include <optional>
#include <utility>

namespace safestore::SafeStoreWrapper
{
    // Free functions used in implementation

    /*
     * Convert an ObjectStatus used by this wrapper to a SafeStore type, SafeStore_ObjectStatus_t.
     * If we cannot map the status then SOS_UNDEFINED is returned.
     */
    SafeStore_ObjectStatus_t convertObjStatusToSafeStoreObjStatus(const ObjectStatus& objectStatus);

    /*
     * Convert an ObjectType used by this wrapper to a SafeStore type, SafeStore_ObjectType_t.
     * If we cannot map the type then SOT_UNKNOWN is returned.
     */
    SafeStore_ObjectType_t convertObjTypeToSafeStoreObjType(const ObjectType& objectType);

    /*
     * Convert a ConfigOption used by this wrapper to a SafeStore type, SafeStore_Config_t.
     * If we cannot map the config option then a default initialised SafeStore_Config_t is returned.
     */
    SafeStore_Config_t convertToSafeStoreConfigId(const ConfigOption& option);

    /**
     * Convert a UUID string into a SafeStore ID type, SafeStore_Id_t.
     */
    std::optional<SafeStore_Id_t> safeStoreIdFromUuidString(const std::string& uuidString);

    /**
     * Convert SafeStore ID type SafeStore_Id_t to a UUID string.
     */
    std::string uuidStringFromSafeStoreId(const SafeStore_Id_t& id);

    /*
     * Get the string representation of a SafeStore return code, useful for logging.
     */
    std::string safeStoreReturnCodeToString(SafeStore_Result_t);

    class SafeStoreHolder : public ISafeStoreHolder
    {
    public:
        InitReturnCode init(const std::string& dbDirName, const std::string& dbName, const std::string& password)
            override;
        void deinit() override;
        SafeStoreContext getHandle() override;

    private:
        SafeStoreContext m_safeStoreCtx = nullptr;
    };

    class SafeStoreReleaseMethodsImpl : public ISafeStoreReleaseMethods
    {
    public:
        explicit SafeStoreReleaseMethodsImpl(std::shared_ptr<ISafeStoreHolder> safeStoreHolder);
        void releaseObjectHandle(SafeStoreObjectHandle objectHandle) override;
        void releaseSearchHandle(SafeStoreSearchHandle searchHandle) override;

    private:
        std::shared_ptr<ISafeStoreHolder> m_safeStoreHolder;
    };

    class SafeStoreSearchMethodsImpl : public ISafeStoreSearchMethods
    {
    public:
        explicit SafeStoreSearchMethodsImpl(std::shared_ptr<ISafeStoreHolder> safeStoreHolder);
        bool findFirst(
            const SafeStoreFilter& filter,
            SearchHandleHolder& searchHandle,
            ObjectHandleHolder& objectHandle) override;
        bool findNext(SearchHandleHolder& searchHandle, ObjectHandleHolder& objectHandle) override;

    private:
        std::shared_ptr<ISafeStoreHolder> m_safeStoreHolder;
    };

    class SafeStoreGetIdMethodsImpl : public ISafeStoreGetIdMethods
    {
    public:
        explicit SafeStoreGetIdMethodsImpl(std::shared_ptr<ISafeStoreHolder> safeStoreHolder);
        ObjectId getObjectId(SafeStoreObjectHandle objectHandle) override;

    private:
        std::shared_ptr<ISafeStoreHolder> m_safeStoreHolder;
    };

    class SafeStoreWrapperImpl : public ISafeStoreWrapper
    {
    public:
        SafeStoreWrapperImpl();
        ~SafeStoreWrapperImpl() override;
        std::unique_ptr<ObjectHandleHolder> createObjectHandleHolder() override;
        std::unique_ptr<SearchHandleHolder> createSearchHandleHolder() override;
        InitReturnCode initialise(const std::string& dbDirName, const std::string& dbName, const std::string& password)
            override;
        SaveFileReturnCode saveFile(
            const std::string& directory,
            const std::string& filename,
            const ThreatId& threatId,
            const std::string& threatName,
            ObjectHandleHolder& objectHandle) override;
        bool finaliseObject(ObjectHandleHolder& objectHandle) override;
        std::optional<uint64_t> getConfigIntValue(ConfigOption option) override;
        bool setConfigIntValue(ConfigOption option, uint64_t value) override;
        SearchResults find(const SafeStoreFilter& filter) override;
        ObjectId getObjectId(const ObjectHandleHolder& objectHandle) override;
        std::string getObjectName(const ObjectHandleHolder& objectHandle) override;
        bool getObjectHandle(const ObjectId& objectId, std::shared_ptr<ObjectHandleHolder> objectHandle) override;
        ObjectType getObjectType(const ObjectHandleHolder& objectHandle) override;
        ObjectStatus getObjectStatus(const ObjectHandleHolder& objectHandle) override;
        ThreatId getObjectThreatId(const ObjectHandleHolder& objectHandle) override;
        std::string getObjectThreatName(const ObjectHandleHolder& objectHandle) override;
        int64_t getObjectStoreTime(const ObjectHandleHolder& objectHandle) override;
        bool setObjectCustomData(
            ObjectHandleHolder& objectHandle,
            const std::string& dataName,
            const std::vector<uint8_t>& value) override;
        std::vector<uint8_t> getObjectCustomData(const ObjectHandleHolder& objectHandle, const std::string& dataName)
            override;
        bool setObjectCustomDataString(
            ObjectHandleHolder& objectHandle,
            const std::string& dataName,
            const std::string& value) override;
        std::string getObjectCustomDataString(ObjectHandleHolder& objectHandle, const std::string& dataName) override;
        bool restoreObjectById(const ObjectId& objectId) override;
        bool restoreObjectByIdToLocation(const ObjectId& objectId, const std::string& path) override;
        bool restoreObjectsByThreatId(const ThreatId& threatId) override;
        bool deleteObjectById(const ObjectId& objectId) override;
        bool deleteObjectsByThreatId(const ThreatId& threatId) override;

    private:
        std::shared_ptr<ISafeStoreHolder> m_safeStoreHolder;
        std::shared_ptr<ISafeStoreReleaseMethods> m_releaseMethods;
        std::shared_ptr<ISafeStoreSearchMethods> m_searchMethods;
        std::shared_ptr<ISafeStoreGetIdMethods> m_getIdMethods;
    };
} // namespace safestore::SafeStoreWrapper