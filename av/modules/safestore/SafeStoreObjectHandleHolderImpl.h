// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "ISafeStoreWrapper.h"

namespace safestore
{
    class SafeStoreObjectHandleHolderImpl : public ISafeStoreObjectHandleHolder
    {
    public:
        ~SafeStoreObjectHandleHolderImpl() override;
        SafeStoreObjectHandle* getRawHandle() override;

    private:
        SafeStoreObjectHandle m_handle = nullptr;
    };
} // namespace safestore