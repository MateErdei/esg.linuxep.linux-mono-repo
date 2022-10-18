// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "IQuarantineManager.h"
#include "ISafeStoreWrapper.h"

#include "Common/PersistentValue/PersistentValue.h"

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
        void callOnDbError();
        void callOnDbSuccess();
        QuarantineManagerState m_state;
        std::shared_ptr<ISafeStoreWrapper> m_safeStore;
        std::mutex m_interfaceMutex;

        // When a database error is encountered we increment this counter by calling callOnDbError()
        // if this value gets larger than or equal to DB_ERROR_COUNT_THRESHOLD then m_state is set to
        // QuarantineManagerState::CORRUPT
        // On a successful DB operation or on a DB deletion callOnDbSuccess is called to ensure that
        // the count only includes continuous DB errors by resetting states and error counts.
        int m_databaseErrorCount = 0;
        Common::PersistentValue<int> m_dbErrorCountThreshold;
    };
} // namespace safestore
