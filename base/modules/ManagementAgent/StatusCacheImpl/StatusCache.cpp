/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "StatusCache.h"
#include "Logger.h"
#include <Common/FileSystem/IFileSystem.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystemException.h>


using namespace ManagementAgent::StatusCache;

namespace ManagementAgent
{
    namespace StatusCacheImpl
    {

        bool StatusCache::statusChanged(const std::string& appid,
                                        const std::string& statusForComparison)
        {
            auto search = m_statusCache.find(appid);

            if (search == m_statusCache.end())
            {
                updateStatus(appid, statusForComparison);
            }
            else if (search->second == statusForComparison)
            {
                return false;
            }
            else
            {
                updateStatus(appid, statusForComparison);
            }

            return true;
        }

        void StatusCache::loadCacheFromDisk()
        {
            std::string statusCachePath = Common::ApplicationConfiguration::applicationPathManager().getManagementAgentStatusCacheFilePath();
            std::vector<std::string> files = Common::FileSystem::fileSystem()->listFiles(statusCachePath);

            for (auto& file : files)
            {
                std::string filename = Common::FileSystem::basename(file);
                std::size_t pos = filename.find(".xml");

                if (pos != std::string::npos)
                {
                    std::string appId = filename.substr(0, pos);
                    std::string statusContents = Common::FileSystem::fileSystem()->readFile(file);
                    m_statusCache[appId] = statusContents;
                }
            }
        }

        void StatusCache::updateStatus(const std::string& appid,
                                       const std::string& statusForComparison)
        {
            m_statusCache[appid] = statusForComparison;
            std::string statusCachePath = Common::ApplicationConfiguration::applicationPathManager().getManagementAgentStatusCacheFilePath();
            std::string filename = appid + ".xml";
            std::string statusCacheFullFilePath = Common::FileSystem::join(statusCachePath, filename);

            try
            {
                Common::FileSystem::fileSystem()->writeFile(statusCacheFullFilePath, statusForComparison);
            }
            catch(Common::FileSystem::IFileSystemException& e)
            {
                LOGERROR("Failed to persist status to status cache, path: '" << statusCacheFullFilePath << "' with error, " << e.what());
            }
        }

    }
}