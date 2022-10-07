// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "ISafeStoreWrapper.h"

namespace safestore
{
    class SafeStoreSearchHandleHolderImpl : public ISafeStoreSearchHandleHolder
    {
    public:
        explicit SafeStoreSearchHandleHolderImpl(SafeStoreContext safeStoreCtx);
        ~SafeStoreSearchHandleHolderImpl() override;
        SafeStoreSearchHandle* getRawHandle() override;

    private:
        SafeStoreSearchHandle m_handle = nullptr;
        SafeStoreContext m_safeStoreCtx = nullptr;
    };
} // namespace safestore