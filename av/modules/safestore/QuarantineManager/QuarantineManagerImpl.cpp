// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "QuarantineManagerImpl.h"

#include "common/ApplicationPaths.h"
#include "common/SaferStrerror.h"
#include "common/StringUtils.h"
#include "safestore/Logger.h"
#include "safestore/SafeStoreTelemetryConsts.h"
#include "safestore/SafeStoreWrapper/SafeStoreWrapperImpl.h"
#include "scan_messages/ClientScanRequest.h"
#include "scan_messages/MetadataRescan.h"
#include "unixsocket/restoreReportingSocket/RestoreReportingClient.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
#include "Common/UtilityImpl/Uuid.h"
#include "Common/UtilityImpl/WaitForUtils.h"

#include "datatypes/IUuidGenerator.h"
#include <linux/fs.h>

#include <utility>

namespace
{
    std::string quarantineManagerStateToString(const safestore::QuarantineManager::QuarantineManagerState& state)
    {
        switch (state)
        {
            case safestore::QuarantineManager::QuarantineManagerState::INITIALISED:
                return "INITIALISED";
            case safestore::QuarantineManager::QuarantineManagerState::UNINITIALISED:
                return "UNINITIALISED";
            case safestore::QuarantineManager::QuarantineManagerState::CORRUPT:
                return "CORRUPT";
            case safestore::QuarantineManager::QuarantineManagerState::STARTUP:
                return "STARTUP";
        }
        return "UNKNOWN";
    }

    class SafeStoreObjectException : public std::runtime_error
    {
    public:
        explicit SafeStoreObjectException(const std::string& msg) : std::runtime_error{ msg }
        {
        }
    };

    /**
     * Returns the path of the quarantined object
     * @param safeStoreWrapper
     * @param objectHandle
     * @throw SafeStoreObjectException if either location or path failed to be found
     * @return The absolute path of the object
     */
    [[nodiscard]] std::string GetObjectPath(
        safestore::SafeStoreWrapper::ISafeStoreWrapper& safeStoreWrapper,
        safestore::SafeStoreWrapper::ObjectHandleHolder& objectHandle)
    {
        auto objectName = safeStoreWrapper.getObjectName(objectHandle);
        if (objectName.empty())
        {
            throw SafeStoreObjectException("Couldn't get object name");
        }

        auto objectLocation = safeStoreWrapper.getObjectLocation(objectHandle);
        if (objectLocation.empty())
        {
            throw SafeStoreObjectException("Couldn't get object location");
        }
        else if (objectLocation.back() != '/')
        {
            objectLocation += '/';
        }

        return objectLocation + objectName;
    }

    std::string GetObjectCustomDataString(
        safestore::SafeStoreWrapper::ISafeStoreWrapper& safeStoreWrapper,
        safestore::SafeStoreWrapper::ObjectHandleHolder& objectHandle,
        const std::string& dataName)
    {
        auto data = safeStoreWrapper.getObjectCustomDataString(objectHandle, dataName);
        if (data.empty())
        {
            throw SafeStoreObjectException("Couldn't get object custom data '" + dataName + "'");
        }
        return data;
    }

    void SetObjectCustomDataString(
        safestore::SafeStoreWrapper::ISafeStoreWrapper& safeStoreWrapper,
        safestore::SafeStoreWrapper::ObjectHandleHolder& objectHandle,
        const std::string& dataName,
        const std::string& dataString,
        bool throwOnFailure = true)
    {
        if (!safeStoreWrapper.setObjectCustomDataString(objectHandle, dataName, dataString))
        {
            const std::string message = "Couldn't set object custom data '" + dataName + "' to '" + dataString + "'";
            if (throwOnFailure)
            {
                throw SafeStoreObjectException(message);
            }
            else
            {
                LOGWARN(message);
            }
        }
    }
} // namespace

namespace safestore::QuarantineManager
{
    bool savePassword(const std::string& password)
    {
        std::string safeStoreDbDirPath = Plugin::getSafeStoreDbDirPath();
        std::string passwordFilePath = Plugin::getSafeStorePasswordFilePath();
        auto fileSystem = Common::FileSystem::fileSystem();

        try
        {
            if (!fileSystem->isDirectory(safeStoreDbDirPath))
            {
                fileSystem->makedirs(safeStoreDbDirPath);
            }

            fileSystem->writeFile(passwordFilePath, password);
            return true;
        }
        catch (const Common::FileSystem::IFileSystemException& ex)
        {
            LOGERROR("Failed to save Quarantine Manager Password to " << passwordFilePath << " due to: " << ex.what());
            return false;
        }
    }

    std::string generatePassword()
    {
        LOGDEBUG("Generating SafeStore password");
        std::string pw = datatypes::uuidGenerator().generate();
        if (pw.length() > SAFESTORE_PASSWORD_MAX_SIZE)
        {
            try
            {
                pw.substr(0, SAFESTORE_PASSWORD_MAX_SIZE - 1);
            }
            catch (const std::exception& ex)
            {
                pw = "";
                LOGERROR("Failed to trim SafeStore database password");
            }
        }
        return pw;
    }

    std::optional<std::string> loadPassword()
    {
        std::string passwordFilePath = Plugin::getSafeStorePasswordFilePath();
        auto fileSystem = Common::FileSystem::fileSystem();
        try
        {
            if (fileSystem->isFile(passwordFilePath))
            {
                return fileSystem->readFile(passwordFilePath);
            }
            LOGDEBUG("SafeStore password file does not exist: " << passwordFilePath);
        }
        catch (const Common::FileSystem::IFileSystemException& ex)
        {
            LOGERROR(
                "Failed to read SafeStore Database Password from " << passwordFilePath << " due to: " << ex.what());
        }
        return std::nullopt;
    }

    QuarantineManagerImpl::QuarantineManagerImpl(
        std::unique_ptr<SafeStoreWrapper::ISafeStoreWrapper> safeStoreWrapper,
        Common::SystemCallWrapper::ISystemCallWrapperSharedPtr  sysCallWrapper,
        ISafeStoreResources& safeStoreResources) :
        m_state(QuarantineManagerState::STARTUP),
        m_safeStore(std::move(safeStoreWrapper)),
        m_dbErrorCountThreshold(Plugin::getPluginVarDirPath(), "safeStoreDbErrorThreshold", 10),
        m_sysCallWrapper{ std::move(sysCallWrapper) },
        safeStoreResources_{ safeStoreResources }
    {
    }

    QuarantineManagerState QuarantineManagerImpl::getState()
    {
        std::lock_guard<std::mutex> lock(m_interfaceMutex);
        return m_state;
    }

    void QuarantineManagerImpl::setState(const QuarantineManagerState& newState)
    {
        if (m_state != newState)
        {
            m_state = newState;
            try
            {
                std::string dormantFlag = Plugin::getSafeStoreDormantFlagPath();
                auto tempDir = Common::ApplicationConfiguration::applicationPathManager().getTempPath();
                switch (m_state)
                {
                    case QuarantineManagerState::INITIALISED:
                        m_fileSystem->removeFile(dormantFlag, true);
                        break;
                    case QuarantineManagerState::UNINITIALISED:
                        m_fileSystem->writeFileAtomically(dormantFlag, "SafeStore database uninitialised", tempDir);
                        break;
                    case QuarantineManagerState::CORRUPT:
                        m_fileSystem->writeFileAtomically(dormantFlag, "SafeStore database corrupt", tempDir);
                        break;
                    case QuarantineManagerState::STARTUP:
                        break;
                }
            }
            catch (const std::exception& exception)
            {
                LOGWARN("Failed to update dormant flag due to: " << exception.what());
            }
        }
    }

    void QuarantineManagerImpl::initialise()
    {
        std::lock_guard<std::mutex> lock(m_interfaceMutex);
        if (m_state == QuarantineManagerState::INITIALISED)
        {
            LOGDEBUG("Quarantine Manager already initialised, not initialising again");
            return;
        }

        LOGDEBUG("Initialising Quarantine Manager");
        auto dbDir = Plugin::getSafeStoreDbDirPath();
        auto dbname = Plugin::getSafeStoreDbFileName();
        auto pw = loadPassword();
        if (!pw.has_value())
        {
            pw = generatePassword();
            if (!pw->empty() && savePassword(pw.value()))
            {
                LOGDEBUG("Successfully saved SafeStore database password to file");
            }
            else
            {
                LOGERROR("Failed to store Quarantine Manager password");
                setState(QuarantineManagerState::UNINITIALISED);
                return;
            }
        }

        auto initResult = m_safeStore->initialise(dbDir, dbname, pw.value());

        if (initResult == SafeStoreWrapper::InitReturnCode::OK)
        {
            setState(QuarantineManagerState::INITIALISED);
            m_databaseErrorCount = 0;
            LOGINFO("Quarantine Manager initialised OK");

            // Update SafeStore config if config file exists
            parseConfig();
        }
        else
        {
            setState(QuarantineManagerState::UNINITIALISED);

            LOGERROR("Quarantine Manager failed to initialise");
            // These return codes won't be resolved by deleting the SafeStore database, so don't count them as database
            // errors
            if (!(initResult == SafeStoreWrapper::InitReturnCode::INVALID_ARG ||
                  initResult == SafeStoreWrapper::InitReturnCode::UNSUPPORTED_OS ||
                  initResult == SafeStoreWrapper::InitReturnCode::UNSUPPORTED_VERSION ||
                  initResult == SafeStoreWrapper::InitReturnCode::OUT_OF_MEMORY))
            {
                callOnDbError();
            }
        }
    }

    common::CentralEnums::QuarantineResult QuarantineManagerImpl::quarantineFile(
        const std::string& filePath,
        const std::string& threatId,
        const std::string& threatType,
        const std::string& threatName,
        const std::string& threatSha256,
        const std::string& sha256,
        const std::string& correlationId,
        datatypes::AutoFd autoFd)
    {
        const std::string escapedPath = common::escapePathForLogging(filePath);
        LOGINFO("Attempting to quarantine " << escapedPath << " with threat '" << threatName << "'");

        if (!Common::UtilityImpl::Uuid::IsValid(threatId))
        {
            LOGERROR(
                "Cannot quarantine " << escapedPath << " because threat ID (" << threatId << ") is not a valid UUID");
            return common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE;
        }

        try
        {
            auto fp = Common::FileSystem::filePermissions();
            if (fp->getInodeFlags(filePath) & FS_IMMUTABLE_FL)
            {
                LOGWARN("File at location: " << escapedPath << " is immutable. Will not quarantine.");
                return common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE;
            }
        }
        catch (const Common::FileSystem::IFileSystemException& ex)
        {
            LOGWARN(
                "Unable to determine if detected file is immutable or not, due to: "
                << common::escapePathForLogging(ex.what()) << ". Will continue quarantine attempt.");
        }

        std::lock_guard<std::mutex> lock(m_interfaceMutex);
        if (m_state != QuarantineManagerState::INITIALISED)
        {
            LOGERROR(
                "Cannot quarantine " << escapedPath << ", SafeStore is in " << quarantineManagerStateToString(m_state)
                                     << " state");
            return common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE;
        }

        // dirName strips trailing separator, but SafeStore doesn't care about it. Adding it resolves parent dir being
        // root.
        std::string directory = Common::FileSystem::dirName(filePath) + "/";
        std::string filename = Common::FileSystem::basename(filePath);

        const std::string escapedDirectory = common::escapePathForLogging(directory);

        datatypes::AutoFd directoryFd(m_fileSystem->getFileInfoDescriptor(directory));
        if (!directoryFd.valid())
        {
            LOGERROR(
                "Cannot quarantine " << escapedPath << " because directory " << escapedDirectory << " does not exist");
            return common::CentralEnums::QuarantineResult::NOT_FOUND;
        }

        const int fd = autoFd.get();
        LOGDEBUG("File Descriptor: " << fd);
        auto pathFromFd = m_fileSystem->readlink("/proc/self/fd/" + std::to_string(fd));

        if (!pathFromFd.has_value())
        {
            LOGERROR("Cannot quarantine " << escapedPath << " as it can't be verified to be the threat");
            return common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE;
        }
        if (pathFromFd.value() == filePath + " (deleted)")
        {
            datatypes::AutoFd fdFromDir(
                m_fileSystem->getFileInfoDescriptorFromDirectoryFD(directoryFd.get(), filename + " (deleted)"));
            if (fdFromDir.valid() && m_fileSystem->compareFileDescriptors(fdFromDir.get(), autoFd.get()))
            {
                LOGERROR("Cannot quarantine " << escapedPath << " as it was moved");
                return common::CentralEnums::QuarantineResult::NOT_FOUND;
            }
            else
            {
                LOGINFO("Don't need to quarantine " << escapedPath << " as it is already deleted");
                return common::CentralEnums::QuarantineResult::SUCCESS;
            }
        }
        else if (pathFromFd.value() != filePath)
        {
            LOGERROR("Cannot quarantine " << escapedPath << " as it was moved");
            return common::CentralEnums::QuarantineResult::NOT_FOUND;
        }

        auto objectHandle = m_safeStore->createObjectHandleHolder();
        auto saveResult = m_safeStore->saveFile(directory, filename, threatId, threatName, *objectHandle);
        if (saveResult == SafeStoreWrapper::SaveFileReturnCode::OK)
        {
            callOnDbSuccess();

            datatypes::AutoFd fd2(m_fileSystem->getFileInfoDescriptorFromDirectoryFD(directoryFd.get(), filename));
            if (!fd2.valid())
            {
                LOGWARN("Cannot quarantine " << escapedPath << " as it does not exist");
                return common::CentralEnums::QuarantineResult::NOT_FOUND;
            }
            if (m_fileSystem->compareFileDescriptors(autoFd.get(), fd2.get()))
            {
                try
                {
                    m_fileSystem->unlinkFileUsingDirectoryFD(directoryFd.get(), filename);
                }
                catch (const Common::FileSystem::IFileSystemException& ex)
                {
                    LOGERROR("Failed to remove " << escapedPath << " while quarantining due to: " << ex.what());
                    return common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE;
                }
            }
            else
            {
                LOGWARN("Won't quarantine " << escapedPath << " because it appears to be a different file");
                return common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE;
            }

            m_safeStore->setObjectCustomDataString(*objectHandle, "SHA256", sha256);
            LOGDEBUG("File SHA256: " << sha256);

            StoreThreats(
                *objectHandle,
                { {
                    .type = threatType,
                    .name = threatName,
                    .sha256 = threatSha256,
                } });

            try
            {
                storeCorrelationId(*objectHandle, correlationId);
                LOGDEBUG("Correlation ID: " << correlationId);
            }
            catch (const std::exception& e)
            {
                LOGERROR("Failed to store correlation ID " << correlationId << ": " << e.what());
                return common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE;
            }

            LOGDEBUG("Finalising file: " << escapedPath);
            if (!m_safeStore->finaliseObject(*objectHandle))
            {
                LOGWARN("Failed to finalise file in SafeStore database: " << escapedPath);
            }
            else
            {
                LOGINFO("Quarantined " << escapedPath << " successfully");
            }

            // Search through the database for any objects with the same threatId
            SafeStoreWrapper::SafeStoreFilter filter;
            filter.objectType = SafeStoreWrapper::ObjectType::FILE;
            filter.threatId = threatId;

            auto objects = m_safeStore->find(filter);
            if (objects.size() > 1)
            {
                LOGINFO(escapedPath << " has been quarantined before; removing redundant copies from quarantine");

                const auto newObjectId = m_safeStore->getObjectId(*objectHandle);

                for (const auto& result : objects)
                {
                    const auto objectId = m_safeStore->getObjectId(result);
                    if (objectId == newObjectId)
                    {
                        continue;
                    }
                    LOGDEBUG("Removing object with ID " << objectId);
                    if (!m_safeStore->deleteObjectById(objectId))
                    {
                        LOGWARN("Failed to remove a redundant quarantined object with ID " << objectId);
                    }
                }
            }

            return common::CentralEnums::QuarantineResult::SUCCESS;
        }
        else
        {
            LOGERROR(
                "Failed to quarantine " << escapedPath
                                        << " due to: " << SafeStoreWrapper::GL_SAVE_FILE_RETURN_CODES.at(saveResult));

            // Only call db error func if the return code is not one of these, which are unrelated to a DB error.
            if (!(saveResult == SafeStoreWrapper::SaveFileReturnCode::INVALID_ARG ||
                  saveResult == SafeStoreWrapper::SaveFileReturnCode::OUT_OF_MEMORY ||
                  saveResult == SafeStoreWrapper::SaveFileReturnCode::MAX_STORE_SIZE_EXCEEDED ||
                  saveResult == SafeStoreWrapper::SaveFileReturnCode::MAX_OBJECT_SIZE_EXCEEDED))
            {
                callOnDbError();
            }
        }

        return common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE;
    }

    bool QuarantineManagerImpl::deleteDatabase()
    {
        std::lock_guard<std::mutex> lock(m_interfaceMutex);
        LOGWARN("Deleting Quarantine database");
        std::string safeStoreDbDir = Plugin::getSafeStoreDbDirPath();
        try
        {
            if (m_fileSystem->exists(safeStoreDbDir))
            {
                m_fileSystem->removeFileOrDirectory(safeStoreDbDir);
                m_fileSystem->makedirs(safeStoreDbDir);
                auto fp = Common::FileSystem::filePermissions();
                fp->chown(safeStoreDbDir, "root", "root");
                fp->chmod(safeStoreDbDir, S_IRUSR | S_IWUSR | S_IXUSR); // 0700
                setState(QuarantineManagerState::UNINITIALISED);
                m_databaseErrorCount = 0;
                Common::Telemetry::TelemetryHelper::getInstance().increment(telemetrySafeStoreDatabaseDeletions, 1ul);
                LOGDEBUG("Quarantine database deletion successful");
            }
            return true;
        }
        catch (const Common::FileSystem::IFileSystemException& ex)
        {
            LOGERROR("Failed to remove Quarantine Database due to: " << ex.what());
            return false;
        }
    }

    std::optional<FdsObjectIdsPair> QuarantineManagerImpl::extractQuarantinedFile(
        SafeStoreWrapper::ObjectHandleHolder threatToExtract)
    {
        auto fp = Common::FileSystem::filePermissions();

        std::string dirPath = Common::FileSystem::join(Plugin::getPluginVarDirPath(), "tempUnpack");
        try
        {
            m_fileSystem->makedirs(dirPath);
            fp->chown(dirPath, "root", "root");
            fp->chmod(dirPath, S_IRUSR | S_IWUSR);
        }
        catch (Common::FileSystem::IFileSystemException& ex)
        {
            LOGWARN("Failed to setup directory for rescan of threats with error: " << ex.what());
            cleanupUnpackDir(false, dirPath);
            return std::nullopt;
        }

        auto objectId = m_safeStore->getObjectId(threatToExtract);
        bool success = m_safeStore->restoreObjectByIdToLocation(objectId, dirPath);
        auto unpackedFiles = m_fileSystem->listAllFilesInDirectoryTree(dirPath);

        if (unpackedFiles.size() != 1)
        {
            if (unpackedFiles.size() > 1)
            {
                LOGERROR("Failed to clean up previous unpacked file");
            }
            if (unpackedFiles.empty())
            {
                LOGWARN("Failed to unpack threat for rescan");
            }
            cleanupUnpackDir(true, dirPath);
            return std::nullopt;
        }
        if (!success)
        {
            LOGWARN("Failed to restore threat for rescan");
            try
            {
                m_fileSystem->removeFile(unpackedFiles[0]);
                cleanupUnpackDir(false, dirPath);
            }
            catch (Common::FileSystem::IFileSystemException& ex)
            {
                LOGERROR("Failed to clean up threat with error: " << ex.what());
                cleanupUnpackDir(true, dirPath);
            }
            return std::nullopt;
        }
        std::string filepath = unpackedFiles[0];

        try
        {
            fp->chown(filepath, "root", "root");
            fp->chmod(filepath, S_IRUSR);
        }
        catch (Common::FileSystem::IFileSystemException& ex)
        {
            // horrible state here we want to clean up and drop all filedescriptors asap
            LOGERROR("Failed to set correct permissions " << ex.what() << " aborting rescan");
            cleanupUnpackDir(false, dirPath);
            return std::nullopt;
        }

        int fd = m_sysCallWrapper->_open(filepath.c_str(), O_RDONLY, 0);
        auto file = FdsObjectIdsPair(datatypes::AutoFd{ fd }, objectId);

        try
        {
            // remove file as soon as we have fd for it to minimise time on disk
            m_fileSystem->removeFile(filepath);
            cleanupUnpackDir(false, dirPath);
        }
        catch (Common::FileSystem::IFileSystemException& ex)
        {
            LOGERROR("Failed to clean up threat with error: " << ex.what());
            cleanupUnpackDir(true, dirPath);
        }

        return file;
    }

    void QuarantineManagerImpl::cleanupUnpackDir(bool failedToCleanUp, const std::string& dirPath)
    {
        try
        {
            m_fileSystem->removeFileOrDirectory(dirPath);
        }
        catch (Common::FileSystem::IFileSystemException& ex)
        {
            if (failedToCleanUp)
            {
                LOGERROR("Failed to clean up staging location for rescan with error: " << ex.what());
            }
            else
            {
                // this is less of a problem if the directory is empty
                LOGWARN("Failed to clean up staging location for rescan with error: " << ex.what());
            }
        }
    }

    // These are private functions, so they are protected by the mutexes on the public interface functions.
    void QuarantineManagerImpl::callOnDbError()
    {
        ++m_databaseErrorCount;
        LOGTRACE("Incremented SafeStore database error count to: " << m_databaseErrorCount);
        if (m_databaseErrorCount >= m_dbErrorCountThreshold.getValue())
        {
            LOGWARN("SafeStore database is corrupt");
            setState(QuarantineManagerState::CORRUPT);
        }
    }

    void QuarantineManagerImpl::callOnDbSuccess()
    {
        LOGTRACE("Setting SafeStore error count to: 0");
        m_databaseErrorCount = 0;
        setState(QuarantineManagerState::INITIALISED);
    }

    bool QuarantineManagerImpl::scanExtractedFileForThreat(
        FdsObjectIdsPair& file, const std::string& originalFilePath)
    {
        auto fd = std::move(file.first);
        auto objectId = file.second;

        auto scanningClient = safeStoreResources_.CreateScanningClientSocket(Plugin::getScanningSocketPath());

        auto response = scan(*scanningClient, fd.get(), originalFilePath);

        if (!response.getErrorMsg().empty()
            && !common::contains(response.getErrorMsg(), "as it is password protected")
            && !common::contains(response.getErrorMsg(), "not a supported file type"))
        {
            LOGERROR("Error on rescan request: " << response.getErrorMsg());
            return false;
        }

        std::shared_ptr<SafeStoreWrapper::ObjectHandleHolder> objectHandle =
            m_safeStore->createObjectHandleHolder();
        if (!m_safeStore->getObjectHandle(objectId, objectHandle))
        {
            LOGERROR("Couldn't get object handle for: " << objectId);
            return false;
        }

        std::string path{ "<unknown path>" };
        try
        {
            path = GetObjectPath(*m_safeStore, *objectHandle);
        }
        catch (const SafeStoreObjectException& e)
        {
            LOGWARN("Couldn't get path for '" << objectId << "': " << e.what());
        }

        const std::string escapedPath = common::escapePathForLogging(path);

        if (response.allClean())
        {
            LOGDEBUG("Rescan found quarantined file no longer a threat: " << escapedPath);
            return true;
        }
        else
        {
            LOGDEBUG("Rescan found quarantined file still a threat: " << escapedPath);
            std::vector<scan_messages::Threat> threats;
            for (const auto& detection : response.getDetections())
            {
                threats.push_back({ .type = detection.type, .name = detection.name, .sha256 = detection.sha256 });
            }
            StoreThreats(*objectHandle, threats);
        }

        return false;
    }

    std::optional<scan_messages::RestoreReport> QuarantineManagerImpl::restoreFile(const std::string& objectId)
    {
        LOGDEBUG("Attempting to restore object: " << objectId);
        // Get all details
        std::shared_ptr<SafeStoreWrapper::ObjectHandleHolder> objectHandle = m_safeStore->createObjectHandleHolder();
        if (!m_safeStore->getObjectHandle(objectId, objectHandle))
        {
            LOGERROR("Couldn't get object handle for: " << objectId << ". Failed to restore.");
            return {};
        }

        std::string path;
        try
        {
            path = GetObjectPath(*m_safeStore, *objectHandle);
        }
        catch (const SafeStoreObjectException& e)
        {
            LOGERROR("Unable to restore " << objectId << ", reason: " << e.what());
            return {};
        }

        const std::string escapedPath = common::escapePathForLogging(path);

        std::string correlationId;
        try
        {
            correlationId = getCorrelationId(*objectHandle);
        }
        catch (const std::exception& e)
        {
            LOGERROR("Unable to restore " << escapedPath << ", couldn't get correlation ID: " << e.what());
            return {};
        }

        // Create report
        scan_messages::RestoreReport restoreReport{ std::time(nullptr), path, correlationId, false };

        // Attempt Restore
        if (!m_safeStore->restoreObjectById(objectId))
        {
            LOGERROR("Unable to restore clean file: " << escapedPath);
            return restoreReport;
        }
        LOGINFO("Restored file to disk: " << escapedPath);

        // Delete file
        if (!m_safeStore->deleteObjectById(objectId))
        {
            LOGWARN("File was restored to disk, but unable to remove it from SafeStore database: " << escapedPath);
            return restoreReport;
        }
        restoreReport.wasSuccessful = true;
        LOGDEBUG("ObjectId successfully deleted from database: " << objectId);
        return restoreReport;
    }

    void QuarantineManagerImpl::rescanDatabase()
    {
        LOGDEBUG("SafeStore Database Rescan request received."); // Log at debug level, since find() will also log at
                                                                 // debug level
        SafeStoreWrapper::SafeStoreFilter filter;                // we want to rescan everything in database
        std::vector<SafeStoreWrapper::ObjectHandleHolder> threatObjects = m_safeStore->find(filter);
        if (threatObjects.empty())
        {
            LOGDEBUG("No threats to rescan");
            return;
        }
        LOGINFO(
            "SafeStore Database Rescan request received. Number of quarantined files to Rescan: "
            << threatObjects.size());

        // We don't have NotifyPipe to initialise the StoppableSleeper to pass here
        auto metadataRescanClient = safeStoreResources_.CreateMetadataRescanClientSocket(
            Plugin::getMetadataRescanSocketPath(), unixsocket::BaseClient::DEFAULT_SLEEP_TIME, {});

        for (auto& objectHandle : threatObjects)
        {
            const auto objectId = m_safeStore->getObjectId(objectHandle);

            std::string filePathForLogging;
            try
            {
                filePathForLogging = common::escapePathForLogging(GetObjectPath(*m_safeStore, objectHandle));
            }
            catch (const SafeStoreObjectException& e)
            {
                LOGWARN("Couldn't get path for '" << objectId << "': " << e.what());
                filePathForLogging = "<unknown path>";
            }

            try
            {
                const bool shouldDoFullRescan = DoMetadataRescan(*metadataRescanClient, objectHandle, objectId);
                if (!shouldDoFullRescan)
                {
                    // This means we are sure the quarantined file is still a threat
                    LOGINFO("Metadata rescan for '" << filePathForLogging << "' found it to still be a threat");
                    continue;
                }
            }
            catch (const SafeStoreObjectException& e)
            {
                // In case of failure, we want to carry on to a full rescan
                LOGWARN("Failed to perform metadata rescan: " << e.what() << ", continuing with full rescan");
            }
            catch (const nlohmann::json::exception& e)
            {
                // In case of failure, we want to carry on to a full rescan
                LOGWARN("Failed to perform metadata rescan: " << e.what() << ", continuing with full rescan");
            }

            const bool shouldRestore = DoFullRescan(objectHandle, objectId, filePathForLogging);
            if (!shouldRestore)
            {
                continue;
            }

            try
            {
                auto restoreReport = restoreFile(objectId);
                if (restoreReport.has_value() && restoreReport->wasSuccessful)
                {
                    Common::Telemetry::TelemetryHelper::getInstance().increment(
                        telemetrySafeStoreSuccessfulFileRestorations, 1ul);
                }
                else
                {
                    Common::Telemetry::TelemetryHelper::getInstance().increment(
                        telemetrySafeStoreFailedFileRestorations, 1ul);
                }

                // Send report
                if (restoreReport.has_value())
                {
                    // TODO LINUXDAR-6396 This whole QM file will be interruptable in the future

                    std::shared_ptr<common::StoppableSleeper> sleeper;
                    auto client = safeStoreResources_.CreateRestoreReportingClient(sleeper);
                    client->sendRestoreReport(restoreReport.value());
                }
            }
            catch (const std::exception& ex)
            {
                LOGERROR("Failed to restore file with object ID " << objectId << ", " << ex.what());
            }
        }
    }

    scan_messages::ScanResponse QuarantineManagerImpl::scan(unixsocket::IScanningClientSocket& socket, int fd, const std::string& originalFilePath)
    {
        scan_messages::ScanResponse response;

        if (socket.socketFd() <= 0)
        {
            auto connectResult = socket.connect();
            if (connectResult != 0)
            {
                response.setErrorMsg("Failed to connect: " + common::safer_strerror(errno));
                return response;
            }
        }

        auto request = std::make_shared<scan_messages::ClientScanRequest>();
        request->setPath(originalFilePath);
        request->setFd(fd);
        request->setScanType(scan_messages::E_SCAN_TYPE_SAFESTORE_RESCAN);
        request->setScanInsideArchives(true);
        request->setScanInsideImages(true);
        request->setUserID("root");

        auto sendResult = socket.sendRequest(request);
        if (!sendResult)
        {
            response.setErrorMsg("Failed to send scan request");
            return response;
        }

        auto receiveResponse = socket.receiveResponse(response);
        if (!receiveResponse)
        {
            response.setErrorMsg("Failed to get scan response");
            return response;
        }

        return response;
    }

    void QuarantineManagerImpl::parseConfig()
    {
        if (m_fileSystem->isFile(Plugin::getSafeStoreConfigPath()))
        {
            LOGINFO("Config file found, parsing optional arguments.");
            try
            {
                auto configContents = m_fileSystem->readFile(Plugin::getSafeStoreConfigPath());
                nlohmann::json j = nlohmann::json::parse(configContents);

                // Due to library limitations, config options must be set in this order to avoid undesired behaviour
                setConfigWrapper(j, SafeStoreWrapper::ConfigOption::AUTO_PURGE);
                setConfigWrapper(j, SafeStoreWrapper::ConfigOption::MAX_OBJECT_SIZE);
                setConfigWrapper(j, SafeStoreWrapper::ConfigOption::MAX_SAFESTORE_SIZE);
                setConfigWrapper(j, SafeStoreWrapper::ConfigOption::MAX_REG_OBJECT_COUNT);
                setConfigWrapper(j, SafeStoreWrapper::ConfigOption::MAX_STORED_OBJECT_COUNT);
            }
            catch (nlohmann::json::parse_error& e)
            {
                LOGERROR("Failed to parse SafeStore config json: " << e.what());
            }
            catch (Common::FileSystem::IFileSystemException& e)
            {
                LOGERROR("Failed to read SafeStore config json: " << e.what());
            }
        }
    }

    void QuarantineManagerImpl::setConfigWrapper(nlohmann::json json, const SafeStoreWrapper::ConfigOption& option)
    {
        if (json.contains(SafeStoreWrapper::GL_OPTIONS_MAP.at(option)) &&
            json[SafeStoreWrapper::GL_OPTIONS_MAP.at(option)].is_number_unsigned())
        {
            m_safeStore->setConfigIntValue(option, json[SafeStoreWrapper::GL_OPTIONS_MAP.at(option)]);
            json.erase(SafeStoreWrapper::GL_OPTIONS_MAP.at(option));
        }
    }

    void QuarantineManagerImpl::storeCorrelationId(
        SafeStoreWrapper::ObjectHandleHolder& objectHandle,
        const std::string& correlationId)
    {
        if (!Common::UtilityImpl::Uuid::IsValid(correlationId))
        {
            throw std::invalid_argument("Invalid correlation ID " + correlationId);
        }

        if (!m_safeStore->setObjectCustomDataString(objectHandle, "correlationId", correlationId))
        {
            throw std::runtime_error(
                "Failed to set SafeStore object custom string 'correlationId' to " + correlationId);
        }
    }

    std::string QuarantineManagerImpl::getCorrelationId(SafeStoreWrapper::ObjectHandleHolder& objectHandle)
    {
        std::string correlationId = m_safeStore->getObjectCustomDataString(objectHandle, "correlationId");
        if (correlationId.empty())
        {
            throw std::runtime_error("Failed to get SafeStore object custom string 'correlationId'");
        }

        if (!Common::UtilityImpl::Uuid::IsValid(correlationId))
        {
            throw std::runtime_error("Saved correlation ID " + correlationId + " is invalid");
        }

        return correlationId;
    }

    bool QuarantineManagerImpl::waitForFilesystemLock(double timeoutSeconds)
    {
        auto dbLockPath = Plugin::getSafeStoreDbLockDirPath();
        auto fs = Common::FileSystem::fileSystem();
        double checkInterval = 0.2;
        auto lockDoesNotExist = Common::UtilityImpl::waitFor(
            timeoutSeconds, checkInterval, [&dbLockPath, &fs]() { return !fs->exists(dbLockPath); });
        return lockDoesNotExist;
    }

    void QuarantineManagerImpl::removeFilesystemLock()
    {
        try
        {
            Common::FileSystem::fileSystem()->removeFileOrDirectory(Plugin::getSafeStoreDbLockDirPath());
        }
        catch (const std::exception& exception)
        {
            LOGERROR(
                "Failed to delete SafeStore Database lock dir: "
                << common::escapePathForLogging(Plugin::getSafeStoreDbLockDirPath()) << ", " << exception.what());
        }
    }

    void QuarantineManagerImpl::StoreThreats(
        SafeStoreWrapper::ObjectHandleHolder& objectHandle,
        const std::vector<scan_messages::Threat>& threats)
    {
        try
        {
            const auto jsonString = nlohmann::json(threats).dump();
            LOGDEBUG("SafeStore 'threats' string to store in object: " << jsonString);
            SetObjectCustomDataString(*m_safeStore, objectHandle, "threats", jsonString);
        }
        catch (const SafeStoreObjectException& e)
        {
            LOGWARN("Failed to store threats: " << e.what());
        }
    }

    [[nodiscard]] std::vector<scan_messages::Threat> QuarantineManagerImpl::GetThreats(
        SafeStoreWrapper::ObjectHandleHolder& objectHandle)
    {
        const auto jsonString = GetObjectCustomDataString(*m_safeStore, objectHandle, "threats");
        LOGDEBUG("SafeStore 'threats' string loaded from object: " << jsonString);
        return nlohmann::json::parse(jsonString).get<std::vector<scan_messages::Threat>>();
    }

    bool QuarantineManagerImpl::DoFullRescan(
        SafeStoreWrapper::ObjectHandleHolder& objectHandle,
        const SafeStoreWrapper::ObjectId& objectId,
        const std::string& originalFilePath)
    {
        LOGINFO(
            "Performing full rescan of quarantined file (original path '" << originalFilePath << "', object ID '"
                                                                          << objectId << "')");

        auto fileToBeScanned = extractQuarantinedFile(std::move(objectHandle));
        if (!fileToBeScanned.has_value())
        {
            return false;
        }
        return scanExtractedFileForThreat(fileToBeScanned.value(), originalFilePath);
    }

    bool QuarantineManagerImpl::DoMetadataRescan(
        unixsocket::IMetadataRescanClientSocket& metadataRescanClientSocket,
        safestore::SafeStoreWrapper::ObjectHandleHolder& objectHandle,
        const SafeStoreWrapper::ObjectId& objectId)
    {
        const auto sha256 = GetObjectCustomDataString(*m_safeStore, objectHandle, "SHA256");
        const auto filePath = GetObjectPath(*m_safeStore, objectHandle);
        const auto escapedPath = common::escapePathForLogging(filePath);

        LOGINFO(
            "Requesting metadata rescan of quarantined file (original path '" << escapedPath << "', object ID '"
                                                                              << objectId << "')");

        const auto threats = GetThreats(objectHandle);
        if (threats.empty())
        {
            throw SafeStoreObjectException("No threats stored");
        }

        const auto result =
            metadataRescanClientSocket.rescan({ .filePath = filePath, .sha256 = sha256, .threat = threats[0] });
        LOGDEBUG(
            "Received metadata rescan response: '" << scan_messages::MetadataRescanResponseToString(result) << "'");

        switch (result)
        {
            case scan_messages::MetadataRescanResponse::needsFullScan:
            case scan_messages::MetadataRescanResponse::clean:
            case scan_messages::MetadataRescanResponse::undetected:
            case scan_messages::MetadataRescanResponse::failed:
                return true;
            case scan_messages::MetadataRescanResponse::threatPresent:
                return false;
        }

        return true;
    }
} // namespace safestore::QuarantineManager