/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <log4cplus/logger.h>
#include <log4cplus/loglevel.h>

namespace log4cplus
{
    const log4cplus::tstring& supportToStringMethod(log4cplus::LogLevel);
    log4cplus::LogLevel supportFromStringMethod(const log4cplus::tstring&);

} // namespace log4cplus

namespace Common
{
    namespace Logging
    {
        const std::string& LOGFORTEST();
        const std::string& LOGOFFFORTEST(); 
        /**
         *   Function that applies the settings defined in <installdir>/etc/logger.conf
         *   Currently, the options are:
         *     VERBOSITY: which can have any of the following values: OFF, DEBUG, INFO, SUPPORT, WARN, ERROR
         *   This function is meant to be called after logger initialization.
         *
         *   Note: if no VERBOSITY is found either in the logbase session or global session, than DEBUG will be set as
         * default. The reason for this is that there is no reason why in production this value would not be set
         * correctly. But if it was not, then it is more likely that the developer will need to know the reason, hence,
         * having a DEBUG level may help.
         *
         *   Note: The reason it does not require the Logger to be given as input as it relies on the fact the log4cplus
         *   has a singleton instance of the Logger.
         *
         *   Note: special behaviour when the applyGeneralConfig is passed LOGFORTEST. In this case, it is not try to
         * read settings file.
         */
        void applyGeneralConfig(const std::string& logbase);

        /** replacement for log4cplus::Logger::getInstance which ensures that the settings are applied.
            this allows, for example for the config file to target 'inner' logs, for example: pluginapi
         */
        log4cplus::Logger getInstance(const std::string& loggername) noexcept;
        enum SophosLogLevel : log4cplus::LogLevel
        {
            DEBUG = log4cplus::DEBUG_LOG_LEVEL,
            SUPPORT = log4cplus::DEBUG_LOG_LEVEL + 1,
            INFO = log4cplus::INFO_LOG_LEVEL,
            WARN = log4cplus::WARN_LOG_LEVEL,
            ERROR = log4cplus::ERROR_LOG_LEVEL,
            OFF = log4cplus::OFF_LOG_LEVEL
        };

        class LoggerSophosSettings
        {
            /**
             * Reads the config from  <installdir>/etc/logger.conf and returns the settings as should be applied to
             * the given product.
             * For example given the following config file:
             *
             * [global]
             * VERBOSITY=INFO
             * [watchdog]
             * VERBOSITY=SUPPORT
             *
             * LoggerSophosSettings sett_wtd("watchdog");
             * assert( sett_wtd.logLevel() == SUPPORT)
             * LoggerSophosSettings sett_dl("suldownloader");
             * assert( sett_dl.logLevel() == INFO)
             *
             * @param productName: the name of the main product or the plugin. Known names: sophos_managementagent,
             * suldownloader, updatescheduler...
             */
            LoggerSophosSettings();

        public:
            // implement the destructor in cpp to avoid having to add the includes of boost in this header.
            ~LoggerSophosSettings();

            /** The reason to make the LoggerSophosSettings a singleton is to allow a read-once per life-time of the
             * settings. Later, on the initialization of the loggers, they can use this settings to apply specific
             * settings for different components inside a main product.
             *
             *  For example, it is possible to have:
             *  [updatescheduler]
             *  VERBOSITY=INFO
             *  [reactor]
             *  VERBOSITY=DEBUG
             *
             *  which would cause the reactor logger to be setup as DEBUG mode accross entries (even inside
             * updatescheduler).
             *
             * @return
             */
            static LoggerSophosSettings& instance();

            /**
             * Will return true if there is a section in the config file that matches the product name.
             * For example:
             *  [updatescheduler]
             *  VERBOSITY=INFO
             *
             * assert( sett.hasSpecializationFor( "updatescheduler") == true)
             * assert( sett.hasSpecializationFor( "reactor") == false)
             * @param productName the name given to the logger instance
             * @return
             */
            bool hasSpecializationFor(const std::string& productName) const;

            /**
             * Return the log level that should be applied for the logger whose name matches the productName.
             * It follows the following pattern:
             * If there is a specific specialization for the product it returns the <productName>.VERBOSITY equivalent
             * value. If the product is empty, it will return the VERBOSITY associated with global or no association. If
             * no specific specialization for the product is found, it will not change logLevelOut and return false
             * @param productName
             * @return true if logLevelOut has been set; false if the logLevelOut has not been set.
             *         On return logLevelOut can be applied to the Logger.
             *
             */
            bool logLevel(const std::string& productName, log4cplus::LogLevel& logLevelOut) const;

        private:
            friend void applyGeneralConfig(const std::string&);
            static bool InTestMode;

            class LoggerConfigTree;
            std::unique_ptr<LoggerConfigTree> m_configTree;
        };
    } // namespace Logging
} // namespace Common
