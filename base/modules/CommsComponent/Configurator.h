/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <memory>
#include <Common/UtilityImpl/ISophosSplUsers.h>
#include <Common/Logging/LoggerConfig.h>

namespace CommsComponent
{
    class NullConfigurator
    {
    public:
        void applyParentSecurityPolicy() {}

        void applyParentInit() {}

        void applyChildSecurityPolicy() {}

        void applyChildInit() {}
    };


// todo: Create the real CommsConfigurator:


// child log file : /log/name.log
    class CommsConfigurator
    {
    public:
        /*
         * drops to the user not facing network
         */
        static void applyParentSecurityPolicy();

        static void applyParentSecurityPolicy(const std::pair<std::string, std::string> &lowPrivUser);

        /*
         * drops to the user facing network and go to jail
         */
        static void applyChildSecurityPolicy();

        static void applyChildSecurityPolicy(const std::string &chrootdir);

        static void
        applyChildSecurityPolicy(const std::pair<std::string, std::string> &lowPrivUser, const std::string &chrootdir);

        /*
         * setup log4cplus
         */
        static void applyParentInit();

        /*
         * Setup files and directories required for logging
         */
        static void applyChildInit();

    };


    /*
     * To be called before chroot to set up files for logging
     * Sets logger.conf and logs directory
     */
    void setupLoggingFiles(const std::string &logBaseDir);
}