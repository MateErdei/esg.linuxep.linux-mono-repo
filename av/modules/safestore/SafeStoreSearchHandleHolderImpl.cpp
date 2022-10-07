// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SafeStoreSearchHandleHolderImpl.h"

#include "Logger.h"
extern "C"
{
#include "safestore.h"
}

namespace safestore
{
    SafeStoreSearchHandleHolderImpl::SafeStoreSearchHandleHolderImpl(SafeStoreContext safeStoreCtx) :
        m_safeStoreCtx(safeStoreCtx)
    {
    }

    SafeStoreSearchHandleHolderImpl::~SafeStoreSearchHandleHolderImpl()
    {
        if (m_handle != nullptr)
        {
            SafeStore_FindClose(m_safeStoreCtx, m_handle);
            m_handle = nullptr;
            LOGTRACE("Cleaned up SafeStore search handle");
        }
    }

    SafeStoreSearchHandle* SafeStoreSearchHandleHolderImpl::getRawHandle()
    {
        return &m_handle;
    }
} // namespace safestore