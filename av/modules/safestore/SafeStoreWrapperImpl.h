// Copyright 2022, Sophos Limited.  All rights reserved.

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
    std::optional<SafeStore_Id_t> threatIdFromString(const std::string& threatId);

    /*
     * TODO 5675 docs
     */
    std::string stringFromThreatId(const SafeStore_Id_t& id);

    class SafeStoreWrapperImpl : public ISafeStoreWrapper
    {
    public:
        ~SafeStoreWrapperImpl() override;
        std::shared_ptr<ISafeStoreSearchHandleHolder> createSearchHandleHolder() override;
        std::shared_ptr<ISafeStoreObjectHandleHolder> createObjectHandleHolder() override;
        InitReturnCode initialise(const std::string& dbDirName, const std::string& dbName, const std::string& password)
            override;
        SaveFileReturnCode saveFile(
            const std::string& directory,
            const std::string& filename,
            const std::string& threatId,
            const std::string& threatName,
            ISafeStoreObjectHandleHolder& objectHandle) override;
        bool finaliseObject(ISafeStoreObjectHandleHolder& objectHandle) override;
        std::optional<uint64_t> getConfigIntValue(ConfigOption option) override;
        bool setConfigIntValue(ConfigOption option, uint64_t value) override;
        bool findFirst(
            const SafeStoreFilter& filter,
            std::shared_ptr<ISafeStoreSearchHandleHolder> searchHandle,
            std::shared_ptr<ISafeStoreObjectHandleHolder> objectHandle) override;
        bool findNext(
            std::shared_ptr<ISafeStoreSearchHandleHolder> searchHandle,
            std::shared_ptr<ISafeStoreObjectHandleHolder> objectHandle) override;
        std::string getObjectName(std::shared_ptr<ISafeStoreObjectHandleHolder> objectHandle) override;
        std::string getObjectId(std::shared_ptr<ISafeStoreObjectHandleHolder> objectHandle) override;
        bool getObjectHandle(const std::string& threatId, std::shared_ptr<ISafeStoreObjectHandleHolder> objectHandle)
            override;
        SearchIterator find(const SafeStoreFilter& filter) override;

    private:
        SafeStoreContext m_safeStoreCtx;
    };
} // namespace safestore