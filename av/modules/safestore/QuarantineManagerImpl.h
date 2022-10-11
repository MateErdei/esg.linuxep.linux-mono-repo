// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "IQuarantineManager.h"
#include "ISafeStoreWrapper.h"

#include <memory>
#include <mutex>
#include <string>
namespace safestore
{
    class QuarantineManagerImpl : public IQuarantineManager
    {
    public:
        explicit QuarantineManagerImpl(std::shared_ptr<ISafeStoreWrapper> safeStoreWrapper);
        void initialise() override;
        QuarantineManagerState getState() override;
        bool deleteDatabase() override;
        bool quarantineFile(
            const std::string& filePath,
            const std::string& threatId,
            const std::string& threatName,
            const std::string& sha256,
            datatypes::AutoFd autoFd) override;

    private:
        QuarantineManagerState m_state;
        std::shared_ptr<ISafeStoreWrapper> m_safeStore;
        std::mutex m_interfaceMutex;
    };
} // namespace safestore
