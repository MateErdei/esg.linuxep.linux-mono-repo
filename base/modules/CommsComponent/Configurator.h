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


// todo: Create the real CommsConfigurator:
    struct UserConf
    {
        std::string userName;
        std::string userGroup;
    };

// child log file : /log/name.log
    class CommsConfigurator
    {
        std::string m_chrootDir;
        UserConf m_childUser;
        UserConf m_parentUser;
        std::string m_sophosInstall;
        std::shared_ptr<Common::Logging::FileLoggingSetup> m_logSetup; 

        /*
         * To be called before chroot to set up files for logging
         * Sets logger.conf and logs directory
         */
        void setupLoggingFiles();

    public:
        CommsConfigurator(const std::string &newRoot, UserConf childUser, UserConf parentUser);

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