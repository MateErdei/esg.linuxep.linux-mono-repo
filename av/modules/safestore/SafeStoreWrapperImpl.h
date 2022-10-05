// Copyright 2022, Sophos Limited.  All rights reserved.

#include "ISafeStoreWrapper.h"

extern "C"
{
#include "safestore.h"
}

#include <optional>

namespace safestore
{

    class SafeStoreObjectHandleHolderImpl : public SafeStoreObjectHandleHolder
    {
    public:
        ~SafeStoreObjectHandleHolderImpl() override;
        SafeStoreObjectHandle* get() override;

    private:
        SafeStoreObjectHandle m_handle;
    };

    class SafeStoreSearchHandleHolderImpl : public SafeStoreSearchHandleHolder
    {
    public:
        SafeStoreSearchHandleHolderImpl(SafeStoreContext safeStoreCtx);
        ~SafeStoreSearchHandleHolderImpl() override;
        SafeStoreSearchHandle* get() override;

    private:
        SafeStoreSearchHandle m_handle;
        SafeStoreContext m_safeStoreCtx;
    };

    std::optional<SafeStore_Id_t> threatIdFromString(const std::string& threatId);

    class SafeStoreWrapperImpl : public ISafeStoreWrapper
    {
    public:
        ~SafeStoreWrapperImpl() override;
        InitReturnCode initialise(const std::string& dbDirName, const std::string& dbName, const std::string& password)
            override;
        SaveFileReturnCode saveFile(
            const std::string& directory,
            const std::string& filename,
            const std::string& threatId,
            const std::string& threatName,
            SafeStoreObjectHandleHolder& objectHandle) override;
        std::optional<uint64_t> getConfigIntValue(ConfigOption option) override;
        bool setConfigIntValue(ConfigOption option, uint64_t value) override;
        bool findFirst(
            SafeStoreFilter filter,
            SafeStoreSearchHandleHolder& searchHandle,
            SafeStoreObjectHandleHolder& objectHandle) override;
        bool findNext(SafeStoreSearchHandleHolder& searchHandle, SafeStoreObjectHandleHolder& objectHandle) override;

    private:
        SafeStoreContext m_safeStoreCtx;
    };
} // namespace safestore