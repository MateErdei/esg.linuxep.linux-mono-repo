/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <memory>
#include <Common/UtilityImpl/ISophosSplUsers.h>
#include <Common/Logging/LoggerConfig.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/Logging/FileLoggingSetup.h>
#include <sstream>

namespace CommsComponent
{
    class NullConfigurator
    {
        std::shared_ptr<Common::Logging::ConsoleLoggingSetup> m_console;
    public:
        void applyParentSecurityPolicy();

        void applyChildSecurityPolicy();

    };

    struct UserConf
    {
        std::string userName;
        std::string userGroup;
        std::string logName;
    };

    using ReadOnlyMount = std::pair<std::string, std::string>;

    class CommsConfigurator
    {
        std::string m_chrootDir;
        UserConf m_childUser;
        UserConf m_parentUser;
        std::shared_ptr<Common::Logging::FileLoggingSetup> m_logSetup;
        std::vector<ReadOnlyMount> m_listOfDependencyPairs;
        std::string m_logsBackup;

        /*
         * To be called before chroot to set up files for logging
         * Sets logger.conf and logs directory
         */
        void setupLoggingFiles();

        //use ostream to append logs to be passed to logger once initialised
        void backupLogsAndRemoveChrootDir(std::vector<std::pair<std::string, int>>& out);
        void backupLogs(std::vector<std::pair<std::string, int>>& out);
        void restoreLogs(std::vector<std::pair<std::string, int>>& out);

    public:
        static void clearFilesOlderThan1Hour();

        static std::vector<ReadOnlyMount> getListOfDependenciesToMount();

        static std::vector<std::string> getListOfMountedEntities(const std::string& chrootDir);

        static void cleanDefaultMountedPaths(const std::string& chrootDir);

        static std::string chrootPathForSSPL(const std::string& ssplRootDir);

        enum MountOperation
        {
            MountSucceeded, MountFailed
        };

        static MountOperation mountDependenciesReadOnly(const UserConf& userConf, const std::vector<ReadOnlyMount>&,
                                                        const std::string& chrootDir, std::vector<std::pair<std::string, int>>& out);

        CommsConfigurator(std::string newRoot, UserConf childUser, UserConf parentUser,
                          std::vector<ReadOnlyMount> dependenciesToMount);

        /*
         * drops to the user not facing network and configure log4cplus
         */
        void applyParentSecurityPolicy();

        /*
         * drops to the user facing network and go to jail and configure log4cplus
         */
        void applyChildSecurityPolicy();


    };

}