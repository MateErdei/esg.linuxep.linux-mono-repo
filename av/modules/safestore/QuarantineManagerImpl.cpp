// Copyright 2022, Sophos Limited.  All rights reserved.

#include "QuarantineManagerImpl.h"

#include "Logger.h"
#include "SafeStoreWrapperImpl.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "common/ApplicationPaths.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <optional>
#include <utility>

namespace safestore
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
            LOGERROR("Failed to read SafeStore Database Password from " << passwordFilePath << " due to: " << ex.what());
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
        std::shared_ptr<ObjectHandleHolder> objectHandle = std::make_shared<ObjectHandleHolder>(*m_safeStore);
        auto saveResult = m_safeStore->saveFile(directory, filename, threatId, threatName, *objectHandle);
        if (saveResult == SaveFileReturnCode::OK)
        {
            // TODO LINUXDAR-5677 delete the file

            LOGDEBUG("Finalising file: " << filename);
            if (m_safeStore->finaliseObject(*objectHandle))
            {
                LOGDEBUG("Finalised file: " << filename);
                return true;
            }
            else
            {
                LOGDEBUG("Failed to finalise file: " << filename);
                return false;
            }
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
        auto dbDir = Plugin::getSafeStoreDbDirPath();
        auto dbname = Plugin::getSafeStoreDbFileName();
        auto pw = loadPassword();
        if (!pw.has_value())
        {
            pw = generatePassword();
            if (!pw->empty() && savePassword(pw.value()))
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

        // temp debug
        LOGINFO(dbDir);
        LOGINFO(dbname);
        LOGINFO(pw.value());
        LOGINFO(&m_safeStore);
        LOGINFO(&(*m_safeStore));

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
        std::lock_guard<std::mutex> lock(m_interfaceMutex);
        LOGWARN("Deleting Quarantine database");
        std::string safeStoreDbDir = Plugin::getSafeStoreDbDirPath();
        auto fileSystem = Common::FileSystem::fileSystem();
        try
        {
            if (fileSystem->exists(safeStoreDbDir))
            {
                fileSystem->removeFilesInDirectory(safeStoreDbDir);
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
} // namespace safestore