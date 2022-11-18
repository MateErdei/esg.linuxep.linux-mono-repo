// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "safestore/QuarantineManager/IQuarantineManager.h"
#include "safestore/SafeStoreWrapper/ISafeStoreWrapper.h"

#include <scan_messages/QuarantineResponse.h>

#include "Common/PersistentValue/PersistentValue.h"

#include <memory>
#include <mutex>
#include <string>
namespace safestore::QuarantineManager
{
    class QuarantineManagerImpl : public IQuarantineManager
    {
    public:
        explicit QuarantineManagerImpl(
            std::unique_ptr<safestore::SafeStoreWrapper::ISafeStoreWrapper> safeStoreWrapper);
        void initialise() override;
        QuarantineManagerState getState() override;
        bool deleteDatabase() override;
        common::CentralEnums::QuarantineResult quarantineFile(
            const std::string& filePath,
            const std::string& threatId,
            const std::string& threatName,
            const std::string& sha256,
            datatypes::AutoFd autoFd) override;
        std::vector<FdsObjectIdsPair> extractQuarantinedFiles() override;

    private:
        void callOnDbError();
        void callOnDbSuccess();
        void setState(const QuarantineManagerState& newState);
        QuarantineManagerState m_state;
        std::unique_ptr<safestore::SafeStoreWrapper::ISafeStoreWrapper> m_safeStore;
        std::mutex m_interfaceMutex;

        // When a database error is encountered we increment this counter by calling callOnDbError()
        // if this value gets larger than or equal to DB_ERROR_COUNT_THRESHOLD then m_state is set to
        // QuarantineManagerState::CORRUPT
        // On a successful DB operation or on a DB deletion callOnDbSuccess is called to ensure that
        // the count only includes continuous DB errors by resetting states and error counts.
        int m_databaseErrorCount = 0;
        Common::PersistentValue<int> m_dbErrorCountThreshold;
    };
} // namespace safestore::QuarantineManager
