// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "../SafeStoreWrapper/ISafeStoreWrapper.h"

namespace safestore::QuarantineManager
{
    enum class MetadataRescanResult
    {
        clean,
        hasThreat,
        uncertain
    };

    class IMetadataRescanner
    {
    public:
        virtual ~IMetadataRescanner() = default;

        virtual MetadataRescanResult Rescan(const SafeStoreWrapper::ObjectHandleHolder& objectHandle) = 0;
    };
} // namespace safestore::QuarantineManager
