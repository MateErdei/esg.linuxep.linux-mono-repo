// Copyright 2024 Sophos Limited. All rights reserved.


#include "Logger.h"
#include "Extractor.h"

#include "common/StringUtils.h"
#include "common/ApplicationPaths.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/FileSystem/IFilePermissions.h"
#include "Common/ZipUtilities/ZipUtils.h"
#include "Common/UtilityImpl/StringUtils.h"


#include <cassert>
#include <iostream>
#include <memory>
#include <optional>
#include <sys/stat.h>
#include <fcntl.h>


namespace safestore
{
    Extractor::Extractor(std::unique_ptr<SafeStoreWrapper::ISafeStoreWrapper> safeStoreWrapper): safeStore_(std::move(safeStoreWrapper))
    {
        std::string errorMsg = "Safestore failed to initialise for extraction";
        LOGDEBUG("Initialising Quarantine Manager");
        auto dbDir = Plugin::getSafeStoreDbDirPath();
        auto dbname = Plugin::getSafeStoreDbFileName();
        auto password = loadPassword();
        if (password.has_value())
        {
            auto initResult = safeStore_->initialise(dbDir, dbname, password.value());

            if (initResult == SafeStoreWrapper::InitReturnCode::OK)
            {
                LOGDEBUG("Quarantine Manager initialised OK");
                return;
            }
            else
            {
                LOGERROR(errorMsg);
            }
        }

        sendResponse(errorMsg);
        throw std::runtime_error(errorMsg);

    }

    void Extractor::sendResponse(const std::string& errorMsg)
    {
        if (!errorMsg.empty())
        {
            response_["errorMsg"] = errorMsg;
        }
        std::cout << response_.dump() << std::endl;
    }

    std::optional<std::string> Extractor::loadPassword()
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


    void Extractor::cleanupUnpackDir(bool failedToCleanUp)
    {
        auto fs = Common::FileSystem::fileSystem();
        try
        {
            fs->removeFileOrDirectory(directory_);
        }
        catch (Common::FileSystem::IFileSystemException &ex)
        {
            if (failedToCleanUp)
            {
                LOGERROR("Failed to clean up staging location for extraction with error: " << ex.what());
            } else
            {
                // this is less of a problem if the directory is empty
                LOGWARN("Failed to clean up staging location for extraction with error: " << ex.what());
            }
        }
    }


    std::string Extractor::extractQuarantinedFile(const std::string& path, const std::string& objectId)
    {
        LOGDEBUG("Attempting to extract threat: " << path);
        auto fp = Common::FileSystem::filePermissions();
        auto fs = Common::FileSystem::fileSystem();

        directory_ = Common::FileSystem::join(Plugin::getPluginVarDirPath(), "tempExtract");
        auto filepath = Common::FileSystem::join(directory_, Common::FileSystem::basename(path));
        try
        {
            fs->makedirs(directory_);
            fp->chown(directory_, "root", "root");
            fp->chmod(directory_, S_IRUSR | S_IWUSR);
        }
        catch (Common::FileSystem::IFileSystemException& ex)
        {
            LOGWARN("Failed to setup directory for extraction of threat with error: " << ex.what());
            cleanupUnpackDir(false);
            return "";
        }


        bool success = safeStore_->restoreObjectByIdToLocation(objectId, directory_);
        if (!success)
        {
            LOGWARN("Failed to restore threat for extraction");
            try
            {
                fs->removeFile(filepath);
                cleanupUnpackDir(false);
            }
            catch (Common::FileSystem::IFileSystemException& ex)
            {
                LOGERROR("Failed to clean up threat with error: " << ex.what());
                cleanupUnpackDir(true);
            }
            return "";
        }

        try
        {
            fp->chown(filepath, "root", "root");
            fp->chmod(filepath, S_IRUSR);
        }
        catch (Common::FileSystem::IFileSystemException& ex)
        {
            // horrible state here we want to clean up and drop all filedescriptors asap
            LOGERROR("Failed to set correct permissions " << ex.what() << " aborting threat extraction");
            cleanupUnpackDir(false);
            return "";
        }


        return filepath;
    }


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
            throw SafeStoreObjectException(LOCATION,"Couldn't get object name");
        }

        auto objectLocation = safeStoreWrapper.getObjectLocation(objectHandle);
        if (objectLocation.empty())
        {
            throw SafeStoreObjectException(LOCATION,"Couldn't get object location");
        }
        else if (objectLocation.back() != '/')
        {
            objectLocation += '/';
        }

        return objectLocation + objectName;
    }

    bool isValidChar(char c)
    {
        if (std::isalnum(c))
        {
            return true;
        }
        return false;
    }
    bool Extractor::doesThreatMatch(const std::string& matchSha, const  std::string& threatSha)
    {
        return matchSha.empty() || matchSha == threatSha;
    }

    int Extractor::extract(const std::string& pathFilter, const std::string& sha256, const std::string& password, const std::string& dest)
    {
        if (sha256.empty() && pathFilter.empty())
        {
            std::stringstream errorMSG;
            errorMSG << "No valid sha or threatpath to filter threats by";
            LOGERROR(errorMSG.str());
            sendResponse(errorMSG.str());
            return USER_ERROR;
        }
        // these might be empty strings depending on the inputs to binary
        // if we succesfully find the threat they will be replaced with full values
        response_["file"] = Common::FileSystem::basename(pathFilter);

        if (password.empty())
        {
            std::stringstream errorMSG;
            errorMSG << "No valid password";
            LOGERROR(errorMSG.str());
            sendResponse(errorMSG.str());
            return USER_ERROR;
        }
        if (!pathFilter.empty())
        {
            if (pathFilter.find('/') != std::string::npos && !Common::UtilityImpl::StringUtils::startswith(pathFilter, "/"))
            {
                std::stringstream errorMSG;
                errorMSG << "threatpath: " << pathFilter << " is invalid";
                LOGERROR(errorMSG.str());
                sendResponse(errorMSG.str());
                return USER_ERROR;
            }
            LOGDEBUG("threatFilter: " << pathFilter);
        }
        if (!sha256.empty())
        {
            if (std::find_if_not(std::cbegin(sha256), std::cend(sha256), isValidChar) !=
                std::cend(sha256))
            {
                std::stringstream errorMSG;
                errorMSG << "sha256: " << sha256 << " is invalid";
                LOGERROR(errorMSG.str());
                sendResponse(errorMSG.str());
                return USER_ERROR;
            }
            LOGDEBUG("sha256: " << sha256);

        }
        LOGDEBUG("Password: " << password);
        LOGDEBUG("Destination: " << dest);

        assert(safeStore_);

        safestore::SafeStoreWrapper::SafeStoreFilter filter;


        std::string filePath;
        std::string id;
        bool found = false;

        if (!pathFilter.empty())
        {

            if (pathFilter.find('/') == std::string::npos)
            {
                //threatpath is a filename
                filter.objectName = pathFilter;
            }
            else if (Common::UtilityImpl::StringUtils::endswith(pathFilter, "/"))
            {
                //threatpath is a directory
                filter.objectLocation = pathFilter.substr(0,pathFilter.length()-1);

            }
            else
            {
                //threatpath is an absolute path, relative paths should have been thrown away before this point
                filter.objectName = Common::FileSystem::basename(pathFilter);

                filter.objectLocation = pathFilter.substr(0,pathFilter.find_last_of('/'));
            }
            std::vector<safestore::SafeStoreWrapper::ObjectHandleHolder> threatObjects = safeStore_->find(filter);
            for (auto &objectHandle: threatObjects)
            {
                id = safeStore_->getObjectId(objectHandle);
                std::string threatsha = safeStore_->getObjectCustomDataString(objectHandle, "SHA256");

                filePath = GetObjectPath(*safeStore_, objectHandle);

                if (!doesThreatMatch(sha256,threatsha))
                {
                    continue;
                }
                else
                {
                    found = true;
                    break;
                }
            }
        }
        else
        {
            std::vector<safestore::SafeStoreWrapper::ObjectHandleHolder> threatObjects = safeStore_->find(filter);
            for (auto &objectHandle: threatObjects)
            {
                id = safeStore_->getObjectId(objectHandle);

                if (doesThreatMatch(sha256, safeStore_->getObjectCustomDataString(objectHandle, "SHA256")))
                {
                    found = true;
                    filePath = GetObjectPath(*safeStore_, objectHandle);
                    break;
                }
            }
        }

        if (found)
        {
            response_["file"] = Common::FileSystem::basename(filePath);
            std::string result = extractQuarantinedFile(filePath, id);
            if (result.empty())
            {
                std::string loggingPath = id;
                try
                {
                    loggingPath = common::escapePathForLogging(filePath);
                }
                catch (const SafeStoreObjectException &e)
                {
                    LOGWARN("Couldn't get path for '" << id << "': " << e.what());
                }
                sendResponse("Failed extraction of threat " + loggingPath);
                return INTERNAL_ERROR;
            }
        }
        else
        {
            std::stringstream errorMSG;
            errorMSG << "Failed to find matching threat in safestore database";
            LOGERROR(errorMSG.str());
            sendResponse(errorMSG.str());
            return USER_ERROR;
        }


        auto ret = Common::ZipUtilities::zipUtils().zip(directory_, dest, false, true, password);
        // clean directory
        cleanupUnpackDir(false);
        LOGDEBUG("Successfully extracted " << pathFilter);
        if (ret != 0)
        {
            LOGERROR("Failed to compress threat with error code: " << ret);
            sendResponse("Failed to compress threat");
            return INTERNAL_ERROR;
        }
        sendResponse("");
        return ret;
    }
}


