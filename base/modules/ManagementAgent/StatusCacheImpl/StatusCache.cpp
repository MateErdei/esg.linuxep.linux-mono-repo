// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "StatusCache.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "ManagementAgent/LoggerImpl/Logger.h"
#include <sys/stat.h>

using namespace ManagementAgent::StatusCache;

namespace ManagementAgent::StatusCacheImpl
{
    bool StatusCache::statusChanged(const std::string& appid, const std::string& statusForComparison)
    {
        auto search = m_statusCache.find(appid);

        // If no status already present in cache for appid, then update the status and return true.
        if (search == m_statusCache.end())
        {
            updateStatus(appid, statusForComparison);
        }
        else if (search->second == statusForComparison)
        {
            // If there is a status present in cache for appid and the status matches the one passed in for
            // comparison then it hasn't changed
            return false;
        }
        else
        {
            // If there is a status present in cache for appid and the status does not match the one passed in for
            // comparison then it has changed, update the status and return true.
            updateStatus(appid, statusForComparison);
        }

        return true;
    }

    void StatusCache::loadCacheFromDisk()
    {
        std::string statusCachePath =
            Common::ApplicationConfiguration::applicationPathManager().getManagementAgentStatusCacheFilePath();
        std::vector<std::string> files = Common::FileSystem::fileSystem()->listFiles(statusCachePath);

        for (auto& file : files)
        {
            std::string filename = Common::FileSystem::basename(file);
            std::size_t pos = filename.find(".xml");

            if (pos != std::string::npos)
            {
                std::string appId = filename.substr(0, pos);
                try
                {
                    // TODO LINUXEP-6540: Is loading unvalidated content leaving us open to compromise?
                    std::string statusContents = Common::FileSystem::fileSystem()->readFile(file);
                    m_statusCache[appId] = statusContents;
                }
                catch (Common::FileSystem::IFileSystemException& e)
                {
                    LOGERROR(
                        "Failed to read status file from status cache, file: '" << file << "' with error, "
                                                                                << e.what());
                }
            }
        }
    }

    void StatusCache::updateStatus(const std::string& appid, const std::string& statusForComparison)
    {
        m_statusCache[appid] = statusForComparison;
        std::string statusCachePath =
            Common::ApplicationConfiguration::applicationPathManager().getManagementAgentStatusCacheFilePath();
        std::string filename = appid + ".xml";
        std::string statusCacheFullFilePath = Common::FileSystem::join(statusCachePath, filename);

        try
        {
            if (Common::FileSystem::fileSystem()->exists(statusCacheFullFilePath))
            {
                Common::FileSystem::fileSystem()->removeFile(statusCacheFullFilePath);
            }
            Common::FileSystem::fileSystem()->writeFile(statusCacheFullFilePath, statusForComparison);
            Common::FileSystem::filePermissions()->chmod(statusCacheFullFilePath, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
        }
        catch (Common::FileSystem::IFileSystemException& e)
        {
            LOGERROR(
                "Failed to persist status to status cache, path: '" << statusCacheFullFilePath << "' with error, "
                                                                    << e.what());
        }
    }
} // namespace ManagementAgent::StatusCacheImpl