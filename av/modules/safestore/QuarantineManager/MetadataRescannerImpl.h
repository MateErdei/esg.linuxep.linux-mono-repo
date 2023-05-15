// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "IMetadataRescanner.h"

namespace safestore::QuarantineManager
{
    class MetadataRescanner : public IMetadataRescanner
    {
    public:
        MetadataRescanner(SafeStoreWrapper::ISafeStoreWrapper& safeStoreWrapper);

        MetadataRescanResult Rescan(const SafeStoreWrapper::ObjectHandleHolder& objectHandle) override;

    private:
        SafeStoreWrapper::ISafeStoreWrapper& safeStoreWrapper_;
    };
} // namespace safestore::QuarantineManager
