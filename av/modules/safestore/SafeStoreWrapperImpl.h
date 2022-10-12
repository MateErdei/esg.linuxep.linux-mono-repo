// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "ISafeStoreWrapper.h"

extern "C"
{
#include "safestore.h"
}

#include <optional>
#include <utility>

namespace safestore
{
    // Free functions used in implementation

    /*
     * TODO 5675 docs
     */
    SafeStore_ObjectStatus_t convertObjStatusToSafeStoreObjStatus(const ObjectStatus& objectStatus);

    /*
     * TODO 5675 docs
     */
    SafeStore_ObjectType_t convertObjTypeToSafeStoreObjType(const ObjectType& objectType);

    /*
     * TODO 5675 docs
     */
    SafeStore_Config_t convertToSafeStoreConfigId(const ConfigOption& option);

    /*
     * TODO 5675 docs
     */
    std::optional<SafeStore_Id_t> safeStoreIdFromString(const std::string& threatId);

    /*
     * TODO 5675 docs
     */
    std::string stringFromSafeStoreId(const SafeStore_Id_t& id);

    class SafeStoreWrapperImpl : public ISafeStoreWrapper
    {
    public:
        ~SafeStoreWrapperImpl() override;
        std::unique_ptr<SearchHandleHolder> createSearchHandleHolder() override;
        std::unique_ptr<ObjectHandleHolder> createObjectHandleHolder() override;
        InitReturnCode initialise(const std::string& dbDirName, const std::string& dbName, const std::string& password)
            override;
        SaveFileReturnCode saveFile(
            const std::string& directory,
            const std::string& filename,
            const std::string& threatId,
            const std::string& threatName,
            ObjectHandleHolder& objectHandle) override;
        bool finaliseObject(ObjectHandleHolder& objectHandle) override;
        std::optional<uint64_t> getConfigIntValue(ConfigOption option) override;
        bool setConfigIntValue(ConfigOption option, uint64_t value) override;
        bool findFirst(
            const SafeStoreFilter& filter,
            SearchHandleHolder& searchHandle,
            ObjectHandleHolder& objectHandle) override;
        bool findNext(
            SearchHandleHolder& searchHandle,
            ObjectHandleHolder& objectHandle) override;
        SearchResults find(const SafeStoreFilter& filter) override;
        std::string getObjectName(ObjectHandleHolder& objectHandle) override;
        std::string getObjectId(const ObjectHandleHolder& objectHandle) override;
        bool getObjectHandle(const std::string& objectId, std::shared_ptr<ObjectHandleHolder> objectHandle)
            override;
        ObjectType getObjectType(ObjectHandleHolder& objectHandle) override;
        ObjectStatus getObjectStatus(ObjectHandleHolder& objectHandle) override;
        std::string getObjectThreatId(ObjectHandleHolder& objectHandle) override;
        std::string getObjectThreatName(ObjectHandleHolder& objectHandle) override;
        int64_t getObjectStoreTime(ObjectHandleHolder& objectHandle) override;
        bool setObjectCustomData(
            ObjectHandleHolder& objectHandle,
            const std::string& dataName,
            const std::vector<uint8_t>& value) override;
        std::vector<uint8_t> getObjectCustomData(ObjectHandleHolder& objectHandle, const std::string& dataName)
            override;

        bool setObjectCustomDataString(
            ObjectHandleHolder& objectHandle,
            const std::string& dataName,
            const std::string& value) override;
        std::string getObjectCustomDataString(ObjectHandleHolder& objectHandle, const std::string& dataName) override;
        void releaseObjectHandle(SafeStoreObjectHandle objectHandleHolder) override;
        void releaseSearchHandle(SafeStoreSearchHandle searchHandleHolder) override;

    private:
        SafeStoreContext m_safeStoreCtx = nullptr;
    };
} // namespace safestore