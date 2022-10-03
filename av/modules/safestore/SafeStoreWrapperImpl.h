// Copyright 2022, Sophos Limited.  All rights reserved.

#include "ISafeStoreWrapper.h"

namespace safestore
{
    class SafeStoreWrapperImpl : public ISafeStoreWrapper
    {
    public:
        SafeStoreWrapperImpl();
        ~SafeStoreWrapperImpl() override;
        InitReturnCode initialise(const std::string& dbDirName, const std::string& dbName) override;

    private:
        SafeStore_t m_safeStoreCtx;
    };
}