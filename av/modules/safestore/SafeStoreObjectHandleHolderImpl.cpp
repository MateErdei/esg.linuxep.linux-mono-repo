// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SafeStoreObjectHandleHolderImpl.h"

#include "Logger.h"

extern "C"
{
#include "safestore.h"
}

namespace safestore
{
    SafeStoreObjectHandleHolderImpl::~SafeStoreObjectHandleHolderImpl()
    {
        if (m_handle != nullptr)
        {
            SafeStore_ReleaseObjectHandle(m_handle);
            m_handle = nullptr;
            LOGTRACE("Cleaned up SafeStore object handle");
        }
    }
    SafeStoreObjectHandle* SafeStoreObjectHandleHolderImpl::getRawHandle()
    {
        return &m_handle;
    }
} // namespace safestore