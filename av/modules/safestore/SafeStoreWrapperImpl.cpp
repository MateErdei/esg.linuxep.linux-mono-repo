// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SafeStoreWrapperImpl.h"

#include "Logger.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "common/ApplicationPaths.h"

namespace safestore
{
    std::string generatePassword()
    {
        return "supersecurepassword";
    }

    void savePassword(const std::string& password)
    {
        std::string passwordFilePath = Plugin::getSafeStorePasswordFilePath();
        auto fileSystem = Common::FileSystem::fileSystem();

        try
        {
            fileSystem->writeFile(passwordFilePath, password);
        }
        catch (const Common::FileSystem::IFileSystemException& ex)
        {
            LOGERROR("Failed to save SafeStore Database Password to " << passwordFilePath << "due to: " << ex.what());
        }
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
            LOGERROR("Failed to read SafeStore Database Password from " << passwordFilePath << "due to: " << ex.what());
        }
        return std::nullopt;
    }

    SafeStoreWrapperImpl::SafeStoreWrapperImpl() {}

    SafeStoreWrapperImpl::~SafeStoreWrapperImpl()
    {
        SafeStore_DeInit(m_safeStoreCtx);
    }

    InitReturnCode SafeStoreWrapperImpl::initialise(const std::string& dbDirName, const std::string& dbName)
    {
        std::string password = loadPassword().value();

        auto result = SafeStore_Init(&m_safeStoreCtx,
                       dbDirName.c_str(),
                       dbName.c_str(),
                       reinterpret_cast<const uint8_t*>(password.c_str()),
                       password.size(),
                       0);
        switch (result)
        {
            case SR_OK:
                LOGINFO("Successfully initialised SafeStore database");
                return OK;
            case SR_INVALID_ARG:
                LOGWARN("Failed to initialise SafeStore database: Invalid argument");
                return INVALID_ARG;
            case SR_UNSUPPORTED_OS:
                LOGWARN("Failed to initialise SafeStore database: Operating system is not supported by SafeStore");
                return UNSUPPORTED_OS;
            case SR_UNSUPPORTED_VERSION:
                LOGWARN("Failed to initialise SafeStore database: Opened SafeStore database file's version is not supported");
                return UNSUPPORTED_VERSION;
            case SR_OUT_OF_MEMORY:
                LOGWARN("Failed to initialise SafeStore database: There is not enough memory available to complete the operation");
                return OUT_OF_MEMORY;
            case SR_DB_OPEN_FAILED:
                LOGWARN("Failed to initialise SafeStore database: Could not open the database");
                return DB_OPEN_FAILED;
            case SR_DB_ERROR:
                LOGWARN("Failed to initialise SafeStore database: Database operation failed");
                return DB_ERROR;
            default:
                LOGWARN("Failed to initialise SafeStore database");
                return FAILED;
        }
    }
}