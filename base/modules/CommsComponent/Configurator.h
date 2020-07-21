/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <memory>
#include <Common/UtilityImpl/ISophosSplUsers.h>
#include <Common/Logging/LoggerConfig.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/Logging/FileLoggingSetup.h>

namespace CommsComponent
{
    class NullConfigurator
    {
        std::shared_ptr<Common::Logging::ConsoleLoggingSetup> m_console; 
    public:
        void applyParentSecurityPolicy() {}

        void applyParentInit();

        void applyChildSecurityPolicy() {}

        void applyChildInit();

    };

    const char *GL_SOPHOS_DIRNAME = "sophos-spl-comms";
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

        /*
         * To be called before chroot to set up files for logging
         * Sets logger.conf and logs directory
         */
        void setupLoggingFiles();

        void mountDependenciesReadOnly(UserConf userConf);

        
    public:
        static std::vector<ReadOnlyMount> getListOfDependenciesToMount();
        static std::vector<std::string> getListOfMountedEntities(const std::string& chrootDir); 
        static void cleanDefaultMountedPaths(const std::string & chrootDir);
        static std::string chrootPathForSSPL(const std::string & ssplRootDir); 

        CommsConfigurator(const std::string& newRoot, UserConf childUser, UserConf parentUser,
                          std::vector<ReadOnlyMount> dependenciesToMount);

        /*
         * drops to the user not facing network
         */
        void applyParentSecurityPolicy();

        /*
         * drops to the user facing network and go to jail
         */
        void applyChildSecurityPolicy();


        /*
         * setup log4cplus
         */
        void applyParentInit();

        /*
         * Setup log4cplus
         */
        void applyChildInit();

    };

}