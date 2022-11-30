// Copyright 2022, Sophos Limited. All rights reserved.

#include "QuarantineManagerImpl.h"

#include "common/ApplicationPaths.h"
#include "common/SaferStrerror.h"
#include "common/StringUtils.h"
#include "datatypes/SystemCallWrapper.h"
#include "safestore/Logger.h"
#include "safestore/SafeStoreWrapper/SafeStoreWrapperImpl.h"
#include "scan_messages/ClientScanRequest.h"
#include "unixsocket/threatDetectorSocket/ScanningClientSocket.h"
#include "unixsocket/restoreReportingSocket/RestoreReportingClient.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/UtilityImpl/Uuid.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

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
        std::ostringstream ss;
        boost::uuids::uuid uuid = boost::uuids::random_generator()();
        ss << uuid;
        std::string pw = ss.str();
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
        std::unique_ptr<SafeStoreWrapper::ISafeStoreWrapper> safeStoreWrapper) :
        m_state(QuarantineManagerState::STARTUP),
        m_safeStore(std::move(safeStoreWrapper)),
        m_dbErrorCountThreshold(Plugin::getPluginVarDirPath(), "safeStoreDbErrorThreshold", 10)
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
            auto fileSystem = Common::FileSystem::fileSystem();
            std::string dormantFlag = Plugin::getSafeStoreDormantFlagPath();
            auto tempDir = Common::ApplicationConfiguration::applicationPathManager().getTempPath();
            switch (m_state)
            {
                case QuarantineManagerState::INITIALISED:
                    fileSystem->removeFile(dormantFlag, true);
                    break;
                case QuarantineManagerState::UNINITIALISED:
                    fileSystem->writeFileAtomically(dormantFlag, "SafeStore database uninitialised", tempDir, 0640);
                    break;
                case QuarantineManagerState::CORRUPT:
                    fileSystem->writeFileAtomically(dormantFlag, "SafeStore database corrupt", tempDir, 0640);
                    break;
                case QuarantineManagerState::STARTUP:
                    break;
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
            if (initResult == SafeStoreWrapper::InitReturnCode::DB_ERROR ||
                initResult == SafeStoreWrapper::InitReturnCode::DB_OPEN_FAILED)
            {
                callOnDbError();
            }
        }
    }

    common::CentralEnums::QuarantineResult QuarantineManagerImpl::quarantineFile(
        const std::string& filePath,
        const std::string& threatId,
        const std::string& threatName,
        const std::string& sha256,
        datatypes::AutoFd autoFd)
    {
        const std::string escapedPath = common::escapePathForLogging(filePath);
        LOGINFO("Attempting to quarantine " << escapedPath << " with threat '" << threatName << "'");

        if (!Common::UtilityImpl::Uuid::IsValid(threatId))
        {
            LOGWARN("Cannot quarantine file because threat ID (" << threatId << ") is not a valid UUID");

            return common::CentralEnums::QuarantineResult::NOT_FOUND;
        }

        std::lock_guard<std::mutex> lock(m_interfaceMutex);
        if (m_state != QuarantineManagerState::INITIALISED)
        {
            LOGWARN("Cannot quarantine file, SafeStore is in " << quarantineManagerStateToString(m_state) << " state");
            return common::CentralEnums::QuarantineResult::NOT_FOUND;
        }

        std::string directory = Common::FileSystem::dirName(filePath);
        std::string filename = Common::FileSystem::basename(filePath);

        auto objectHandle = m_safeStore->createObjectHandleHolder();
        auto saveResult = m_safeStore->saveFile(directory, filename, threatId, threatName, *objectHandle);
        if (saveResult == SafeStoreWrapper::SaveFileReturnCode::OK)
        {
            callOnDbSuccess();
            auto fs = Common::FileSystem::fileSystem();
            LOGDEBUG("File Descriptor: " << autoFd.fd());

            datatypes::AutoFd directoryFd(fs->getFileInfoDescriptor(directory));
            if (!directoryFd.valid())
            {
                LOGWARN("Directory of threat does not exist");
                return common::CentralEnums::QuarantineResult::NOT_FOUND;
            }

            datatypes::AutoFd fd2(fs->getFileInfoDescriptorFromDirectoryFD(directoryFd.get(), filename));
            if (!fd2.valid())
            {
                LOGWARN("Threat does not exist at path: " << filePath << " Cannot quarantine it");
                return common::CentralEnums::QuarantineResult::NOT_FOUND;
            }
            if (fs->compareFileDescriptors(autoFd.get(), fd2.get()))
            {
                try
                {
                    fs->removeFileOrDirectory(filePath);
                }
                catch (const Common::FileSystem::IFileSystemException& ex)
                {
                    LOGERROR("Removing " << filePath << " failed due to: " << ex.what());
                    return common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE;
                }
            }
            else
            {
                LOGWARN("Cannot verify file to be quarantined");
                return common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE;
            }

            m_safeStore->setObjectCustomDataString(*objectHandle, "SHA256", sha256);
            LOGDEBUG("File SHA256: " << sha256);

            LOGDEBUG("Finalising file: " << escapedPath);
            if (!m_safeStore->finaliseObject(*objectHandle))
            {
                LOGERROR("Failed to finalise file: " << escapedPath);
                return common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE;
            }
            LOGDEBUG("Finalised file: " << escapedPath);

            LOGINFO("Quarantined " << escapedPath << " successfully");

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
            if (saveResult == SafeStoreWrapper::SaveFileReturnCode::DB_ERROR)
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
        auto fileSystem = Common::FileSystem::fileSystem();
        try
        {
            if (fileSystem->exists(safeStoreDbDir))
            {
                fileSystem->removeFilesInDirectory(safeStoreDbDir);
                setState(QuarantineManagerState::UNINITIALISED);
                m_databaseErrorCount = 0;
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

    std::vector<FdsObjectIdsPair> QuarantineManagerImpl::extractQuarantinedFiles(
        datatypes::ISystemCallWrapper& sysCallWrapper)
    {
        std::vector<FdsObjectIdsPair> files;

        SafeStoreWrapper::SafeStoreFilter filter; // we want to rescan everything in database
        std::vector<SafeStoreWrapper::ObjectHandleHolder> items = m_safeStore->find(filter);
        if (items.empty())
        {
            LOGDEBUG("No threats to rescan");
            return files;
        }

        auto fs = Common::FileSystem::fileSystem();
        auto fp = Common::FileSystem::filePermissions();

        std::string dirPath = Common::FileSystem::join(Plugin::getPluginVarDirPath(), "tempUnpack");
        try
        {
            fs->makedirs(dirPath);
            fp->chown(dirPath, "root", "root");
            fp->chmod(dirPath, S_IRUSR | S_IWUSR);
        }
        catch (Common::FileSystem::IFileSystemException& ex)
        {
            LOGWARN("Failed to setup directory for rescan of threats with error: " << ex.what());
            return files;
        }

        bool failedToCleanUp = false;
        for (const auto& item : items)
        {
            SafeStoreWrapper::ObjectId objectId = m_safeStore->getObjectId(item);
            bool success = m_safeStore->restoreObjectByIdToLocation(objectId, dirPath);
            std::vector<std::string> unpackedFiles = fs->listAllFilesInDirectoryTree(dirPath);

            if (unpackedFiles.size() > 1)
            {
                LOGERROR("Failed to clean up previous unpacked file");
                failedToCleanUp = true;
                break;
            }
            if (unpackedFiles.empty())
            {
                LOGWARN("Failed to unpack threat for rescan");
                continue;
            }
            if (!success)
            {
                LOGWARN("Failed to restore threat for rescan");
                try
                {
                    fs->removeFile(unpackedFiles[0]);
                }
                catch (Common::FileSystem::IFileSystemException& ex)
                {
                    LOGERROR("Failed to clean up threat with error: " << ex.what());
                    failedToCleanUp = true;
                    break;
                }
                continue;
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
                fs->removeFileOrDirectory(dirPath);
                return {};
            }

            int fd = sysCallWrapper._open(filepath.c_str(), O_RDONLY, 0);
            files.emplace_back(datatypes::AutoFd{ fd }, objectId);

            try
            {
                // remove file as soon as we have fd for it to minimise time on disk
                fs->removeFile(filepath);
            }
            catch (Common::FileSystem::IFileSystemException& ex)
            {
                LOGERROR("Failed to clean up threat with error: " << ex.what());
                failedToCleanUp = true;
                break;
            }
        }

        try
        {
            fs->removeFileOrDirectory(dirPath);
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

        return files;
    }

    // These are private functions, so they are protected by the mutexes on the public interface functions.
    void QuarantineManagerImpl::callOnDbError()
    {
        ++m_databaseErrorCount;
        if (m_databaseErrorCount >= m_dbErrorCountThreshold.getValue())
        {
            LOGWARN("SafeStore database is corrupt");
            setState(QuarantineManagerState::CORRUPT);
        }
    }

    void QuarantineManagerImpl::callOnDbSuccess()
    {
        m_databaseErrorCount = 0;
        setState(QuarantineManagerState::INITIALISED);
    }

    std::vector<SafeStoreWrapper::ObjectId> QuarantineManagerImpl::scanExtractedFilesForRestoreList(
        std::vector<FdsObjectIdsPair> files)
    {
        std::vector<SafeStoreWrapper::ObjectId> cleanFiles;
        if (files.empty())
        {
            LOGINFO("No files to Rescan");
            return cleanFiles;
        }

        LOGINFO("Number of files to Rescan: " << files.size());

        unixsocket::ScanningClientSocket scanningClient(Plugin::getScanningSocketPath());

        for (auto& [fd, objectId] : files)
        {
            auto response = scan(scanningClient, fd.get());
            fd.close(); // Release the file descriptor as soon as we are done with it

            if (!response.getErrorMsg().empty())
            {
                LOGERROR("Error on rescan request: " << response.getErrorMsg());
                continue;
            }

            std::shared_ptr<SafeStoreWrapper::ObjectHandleHolder> objectHandle =
                m_safeStore->createObjectHandleHolder();
            if (!m_safeStore->getObjectHandle(objectId, objectHandle))
            {
                LOGERROR("Couldn't get object handle for: " << objectId << ", continuing...");
                continue;
            }

            auto objectName = m_safeStore->getObjectName(*objectHandle);
            if (objectName.empty())
            {
                LOGWARN("Couldn't get object name for: " << objectId << ".");
            }

            auto objectLocation = m_safeStore->getObjectLocation(*objectHandle);
            if (objectLocation.empty())
            {
                LOGWARN("Couldn't get object location for: " << objectId << ".");
            }

            const std::string escapedPath = common::escapePathForLogging(objectLocation + "/" + objectName);

            if (response.allClean())
            {
                LOGDEBUG("Rescan found quarantined file no longer a threat: " << escapedPath);
                cleanFiles.emplace_back(objectId);
            }
            else
            {
                LOGDEBUG("Rescan found quarantined file still a threat: " << escapedPath);
            }
        }
        return cleanFiles;
    }

    std::optional<scan_messages::RestoreReport> QuarantineManagerImpl::restoreFile(const std::string& objectId)
    {
        LOGINFO("Attempting to restore object: " << objectId);
        // Get all details
        std::shared_ptr<SafeStoreWrapper::ObjectHandleHolder> objectHandle = m_safeStore->createObjectHandleHolder();
        if (!m_safeStore->getObjectHandle(objectId, objectHandle))
        {
            LOGERROR("Couldn't get object handle for: " << objectId);
            return {};
        }

        auto objectName = m_safeStore->getObjectName(*objectHandle);
        if (objectName.empty())
        {
            LOGWARN("Couldn't get object name for: " << objectId << ".");
        }

        auto objectLocation = m_safeStore->getObjectLocation(*objectHandle);
        if (objectLocation.empty())
        {
            LOGWARN("Couldn't get object location for: " << objectId << ".");
        }

        const std::string path = objectLocation + "/" + objectName;
        const std::string escapedPath = common::escapePathForLogging(path);

        const std::string threatId = m_safeStore->getObjectThreatId(*objectHandle);
        if (threatId.empty())
        {
            LOGERROR("Couldn't get threatId for: " << escapedPath << ", unable to restore.");
            return {};
        }

        // Create report
        scan_messages::RestoreReport restoreReport{ std::time(nullptr), path, threatId, false };

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
            LOGERROR("Unable to remove file from SafeStore database: " << escapedPath);
            return restoreReport;
        }
        restoreReport.wasSuccessful = true;
        return restoreReport;
    }

    void QuarantineManagerImpl::rescanDatabase()
    {
        LOGINFO("SafeStore Database Rescan request received.");
        datatypes::SystemCallWrapper sysCallWrapper;
        auto filesToBeScanned = extractQuarantinedFiles(sysCallWrapper);

        auto objectIdsToRestore = scanExtractedFilesForRestoreList(std::move(filesToBeScanned));
        for (const auto& objectId : objectIdsToRestore)
        {
            auto restoreReport = restoreFile(objectId);
            // Send report
            if (restoreReport.has_value())
            {
                std::shared_ptr<common::StoppableSleeper> sleeper; // TODO - This whole QM file will be interruptable in the future
                unixsocket::RestoreReportingClient client(sleeper);
                client.sendRestoreReport(restoreReport.value());
            }
        }
    }

    scan_messages::ScanResponse QuarantineManagerImpl::scan(unixsocket::ScanningClientSocket& socket, int fd)
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
        auto fileSystem = Common::FileSystem::fileSystem();
        if (fileSystem->isFile(Plugin::getSafeStoreConfigPath()))
        {
            LOGINFO("Config file found, parsing optional arguments.");
            try
            {
                auto configContents = fileSystem->readFile(Plugin::getSafeStoreConfigPath());
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
} // namespace safestore::QuarantineManager