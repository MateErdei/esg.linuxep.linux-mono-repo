// Copyright 2023, Sophos Limited.  All rights reserved.

#pragma once

#include <memory>

#include "common/AbstractThreadPluginInterface.h"

namespace sspl::sophosthreatdetectorimpl
{
    class ISafeStoreRescanWorker : public common::AbstractThreadPluginInterface
    {
    public:
        virtual ~ISafeStoreRescanWorker() = default;

        virtual void triggerRescan() = 0;
        virtual void sendRescanRequest(std::unique_lock<std::mutex>& lock) = 0;
    };

    using ISafeStoreRescanWorkerPtr = std::shared_ptr<ISafeStoreRescanWorker>;
}

