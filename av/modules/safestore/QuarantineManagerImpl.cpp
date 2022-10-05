// Copyright 2022, Sophos Limited.  All rights reserved.

#include "QuarantineManagerImpl.h"

#include "Logger.h"
#include "SafeStoreWrapperImpl.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "common/ApplicationPaths.h"

#include <optional>

namespace safestore
{
    bool savePassword(const std::string& password)
    {
        std::string passwordFilePath = Plugin::getSafeStorePasswordFilePath();
        auto fileSystem = Common::FileSystem::fileSystem();

        try
        {
            fileSystem->writeFile(passwordFilePath, password);
            return true;
        }
        catch (const Common::FileSystem::IFileSystemException& ex)
        {
            LOGERROR("Failed to save SafeStore Database Password to " << passwordFilePath << "due to: " << ex.what());
            return false;
        }
    }

    std::string generatePassword()
    {
        // TODO 5675
        std::string pw = "TEMP PASSWORD";
        //        .....
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
            LOGERROR("Failed to read SafeStore Database Password from " << passwordFilePath << "due to: " << ex.what());
        }
        return std::nullopt;
    }

    QuarantineManagerImpl::QuarantineManagerImpl(std::shared_ptr<ISafeStoreWrapper> safeStoreWrapper) :
        m_state(QuarantineManagerState::UNINITIALISED), m_safeStore(std::move(safeStoreWrapper))
    {
    }

    QuarantineManagerState QuarantineManagerImpl::getState()
    {
        std::lock_guard<std::mutex> lock(m_interfaceMutex);
        return m_state;
    }

    bool QuarantineManagerImpl::quarantineFile(
        const std::string& directory,
        const std::string& filename,
        const std::string& threatId,
        const std::string& threatName)
    {
        std::lock_guard<std::mutex> lock(m_interfaceMutex);
        SafeStoreObjectHandleHolderImpl objectHandle;
        auto saveResult = m_safeStore->saveFile(directory, filename, threatId, threatName, objectHandle);
        if (saveResult == SaveFileReturnCode::OK)
        {
            // TODO LINUXDAR-5677 delete the file
            return true;
        }
        else
        {
            // TODO LINUXDAR-5677 handle quarantine failure
        }

        return false;
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
        // TODO use app paths
        auto dbDir = Plugin::getSafeStoreDbDirPath();
        auto dbname = Plugin::getSafeStoreDbFileName();
        auto pw = loadPassword();
        if (!pw.has_value())
        {
            pw = generatePassword();
            if (savePassword(pw.value()))
            {
                LOGDEBUG("Saved password OK");
            }
            else
            {
                LOGERROR("Failed to store Quarantine Manager password");
                m_state = QuarantineManagerState::UNINITIALISED;
                return;
            }
        }

        auto initResult = m_safeStore->initialise(dbDir, dbname, pw.value());

        if (initResult == InitReturnCode::OK)
        {
            m_state = QuarantineManagerState::INITIALISED;
            LOGINFO("Quarantine Manager initialised OK");
        }
        else
        {
            m_state = QuarantineManagerState::UNINITIALISED;
            LOGERROR("Quarantine Manager failed to initialise");
        }
    }

    bool QuarantineManagerImpl::deleteDatabase()
    {
        LOGWARN("Deleting Quarantine database");
        LOGINFO("TODO 5675 - NOT IMPLEMENTED YET");
        return false;
    }
} // namespace safestore