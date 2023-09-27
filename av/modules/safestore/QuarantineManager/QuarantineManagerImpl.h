// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "safestore/ISafeStoreResources.h"
#include "safestore/QuarantineManager/IQuarantineManager.h"
#include "safestore/SafeStoreWrapper/ISafeStoreWrapper.h"
#include "scan_messages/MetadataRescan.h"
#include "scan_messages/QuarantineResponse.h"
#include "scan_messages/ScanResponse.h"
#include "unixsocket/metadataRescanSocket/IMetadataRescanClientSocket.h"
#include "unixsocket/threatDetectorSocket/IScanningClientSocket.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/PersistentValue/PersistentValue.h"
#include "Common/SystemCallWrapper/ISystemCallWrapper.h"

#include <nlohmann/json.hpp>

#include <memory>
#include <mutex>
#include <string>

namespace safestore::QuarantineManager
{
    class QuarantineManagerImpl : public IQuarantineManager
    {
    public:
        QuarantineManagerImpl(
            std::unique_ptr<safestore::SafeStoreWrapper::ISafeStoreWrapper> safeStoreWrapper,
            Common::SystemCallWrapper::ISystemCallWrapperSharedPtr  sysCallWrapper,
            ISafeStoreResources& safeStoreResources);
        void initialise() override;
        QuarantineManagerState getState() override;
        bool deleteDatabase() override;
        common::CentralEnums::QuarantineResult quarantineFile(
            const std::string& filePath,
            const std::string& threatId,
            const std::string& threatType,
            const std::string& threatName,
            const std::string& threatSha256,
            const std::string& sha256,
            const std::string& correlationId,
            datatypes::AutoFd autoFd) override;
        void setState(const safestore::QuarantineManager::QuarantineManagerState& newState) override;
        void rescanDatabase() override;
        void parseConfig() override;
        bool waitForFilesystemLock(double timeoutSeconds) override;
        void removeFilesystemLock() override;
        std::optional<scan_messages::RestoreReport> restoreFile(const std::string& objectId) override;

        bool scanExtractedFileForThreat(FdsObjectIdsPair& file, const std::string& originalFilePath);
        std::optional<FdsObjectIdsPair> extractQuarantinedFile(SafeStoreWrapper::ObjectHandleHolder threatToExtract);
    private:
        void cleanupUnpackDir(bool failedToCleanUp, const std::string& dirPath);
        void callOnDbError();
        void callOnDbSuccess();
        void setConfigWrapper(nlohmann::json json, const safestore::SafeStoreWrapper::ConfigOption& option);
        void storeCorrelationId(SafeStoreWrapper::ObjectHandleHolder& objectHandle, const std::string& correlationId);
        [[nodiscard]] std::string getCorrelationId(SafeStoreWrapper::ObjectHandleHolder& objectHandle);

        /**
         * Stores the threats in object data. Does not throw on failure.
         */
        void StoreThreats(
            SafeStoreWrapper::ObjectHandleHolder& objectHandle,
            const std::vector<scan_messages::Threat>& threats);
        [[nodiscard]] std::vector<scan_messages::Threat> GetThreats(SafeStoreWrapper::ObjectHandleHolder& objectHandle);

        /**
         * @returns true if the file can be restored
         */
        [[nodiscard]] bool DoFullRescan(
            safestore::SafeStoreWrapper::ObjectHandleHolder& objectHandle,
            const SafeStoreWrapper::ObjectId& objectId,
            const std::string& originalFilePath);
        /**
         * Determines if an object should be fully rescanned.
         * By itself can't determine if the file can be restored, which was a design decision.
         * Technically, in certain cases it should be possible make a decision to restore a file just on the metadata
         * rescan result, however, we have decided not to do that and always fully rescan restoration candidates.
         * @returns true if a full rescan should be done on the object
         */
        [[nodiscard]] bool DoMetadataRescan(
            unixsocket::IMetadataRescanClientSocket& metadataRescanClientSocket,
            safestore::SafeStoreWrapper::ObjectHandleHolder& objectHandle,
            const SafeStoreWrapper::ObjectId& objectId);

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
        static scan_messages::ScanResponse scan(unixsocket::IScanningClientSocket& socket, int fd, const std::string& originalFilePath);
        Common::SystemCallWrapper::ISystemCallWrapperSharedPtr  m_sysCallWrapper;
        ISafeStoreResources& safeStoreResources_;
        Common::FileSystem::IFileSystem* m_fileSystem = Common::FileSystem::fileSystem();
    };
} // namespace safestore::QuarantineManager
