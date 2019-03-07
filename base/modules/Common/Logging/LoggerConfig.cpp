/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "LoggerConfig.h"
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <iostream>
namespace
{
    /**
     * Translate the settings name <string> into its equivalent method.
     * @param logLevelName
     * @param logLevel (OUTPUT)
     * @return true: if the logLevelName was valid. otherwise it returns false and does not change logLevel
     */
    bool fromNameToLog(const std::string & logLevelName, log4cplus::LogLevel & logLevel)
    {
        using sp = Common::Logging::SophosLogLevel;
        static std::vector<std::string> LogNames{{"OFF"}, {"DEBUG"}, {"INFO"}, {"SUPPORT"}, {"WARN"}, {"ERROR"}};
        static std::vector<sp> LogLevels{sp::OFF, sp::DEBUG, sp::INFO, sp::SUPPORT, sp::WARN, sp::ERROR};

        auto ind_it = std::find( LogNames.begin(), LogNames.end(), logLevelName);
        if ( ind_it == LogNames.end())
        {
            return false;
        }
        assert( std::distance(LogNames.begin(), ind_it) >= 0);
        assert( std::distance(LogNames.begin(), ind_it) < static_cast<int>(LogLevels.size()));
        logLevel = LogLevels.at( std::distance(LogNames.begin(), ind_it));
        return true;
    }
}

const std::string LOGFORTEST{"LOGFORTEST"};

void Common::Logging::applyGeneralConfig(const std::string& logbase)
{
    log4cplus::LogLevel logLevel{SophosLogLevel::DEBUG}; // default value
    if ( logbase == LOGFORTEST)
    {
        LoggerSophosSettings::InTestMode = true;
    }

    if( LoggerSophosSettings::instance().hasSpecializationFor(logbase))
    {
        // the fact that it may return false can be ignored here since, in this case the default is to be applied anyway
        (void) LoggerSophosSettings::instance().logLevel(logbase, logLevel);
    }
    else
    {
        // get the global configuration. Again, it may not exist, but in this case the default is to be used.
        (void) LoggerSophosSettings::instance().logLevel("", logLevel);
    }

    log4cplus::Logger::getRoot().setLogLevel(logLevel);
}

log4cplus::Logger Common::Logging::getInstance(const std::string& loggername)
{
    log4cplus::Logger logger =  log4cplus::Logger::getInstance(LOG4CPLUS_TEXT(loggername));
    // this implementation relies on log4cplus inheritance of settings and hence
    // change loglevel only if there is a specific specialization associated with the
    // log name.

    if ( LoggerSophosSettings::instance().hasSpecializationFor(loggername))
    {
        log4cplus::LogLevel logLevel;
        // logLevel may not be set if the value is not recognized.
        if ( LoggerSophosSettings::instance().logLevel(loggername, logLevel))
        {
            logger.setLogLevel(logLevel);
        }
    }

    return logger;

}


namespace Common
{
    namespace Logging
    {
        bool LoggerSophosSettings::InTestMode{false};

        namespace pt = boost::property_tree;
        class LoggerSophosSettings::LoggerConfigTree
        {
        public:

            /* it may throw if the content of the file is invalid or if the file can not be accessed
             *  The reason to use the filePath and not the file content is to avoid a circular dependency.
             *
             *  For tests purpose, I would like the capability of mocking out the file and probaly use the Common::FileSystem
             *  to change content. The problem is that filessytem depends on the logging, hence, logging must not depend on filessytem
             *
             */
            LoggerConfigTree( const std::string & confFilePath);


            /** if session is empty, it will find the verbosity defined for global or no session associated.
             *
             * return empty string if VERBOSITY does not exist in the session
             * */
            std::string getVerbosity( const std::string & session = "") const;

            bool hasSession(const std::string & session) const;


        private:
            static const std::string VERBOSITY;
            pt::ptree m_ptree;
        };
        const std::string LoggerSophosSettings::LoggerConfigTree::VERBOSITY{"VERBOSITY"};



        LoggerSophosSettings::LoggerConfigTree::LoggerConfigTree(const std::string& confFilePath)
        {
            if ( confFilePath.empty())
            {
                return;
            }
            // uses fstream as logging can not depend on Common::FileSystem
            int backup_errno = errno;
            std::fstream i(confFilePath);
            if (i)
            {
                // Parse the XML into the property tree.
                pt::read_ini(i, m_ptree);
            }
            else
            {
                int currErrno = errno;
                std::stringstream s;
                s << "Invalid path for log config: " << confFilePath << ". Err: " << strerror(currErrno);
                errno = backup_errno;
                throw std::runtime_error(s.str());
            }
        }


        std::string LoggerSophosSettings::LoggerConfigTree::getVerbosity( const std::string & session) const
        {

            boost::optional<std::string> value;
            if( session.empty())
            {
                value = m_ptree.get_optional<std::string>("global."+VERBOSITY);
                if (value)
                {
                    return value.get();
                }
                value = m_ptree.get_optional<std::string>(VERBOSITY);
                if ( value )
                {
                    return value.get();
                }
            }
            else
            {
                value = m_ptree.get_optional<std::string>(session + "." + VERBOSITY );
                if ( value)
                {
                    return value.get();
                }
            }
            return std::string{};
        }

        bool  LoggerSophosSettings::LoggerConfigTree::hasSession(const std::string& session) const
        {
            auto session_tree = m_ptree.get_child_optional(session);
            return bool(session_tree);
        }


        LoggerSophosSettings::LoggerSophosSettings()
        {
            if ( LoggerSophosSettings::InTestMode)
            {
                // do not try to load the file property if TestMode = true
                return;
            }
            auto & pathMan=Common::ApplicationConfiguration::applicationPathManager();

            try
            {
                m_configTree = std::unique_ptr<LoggerConfigTree>(new LoggerConfigTree( pathMan.getLogConfFilePath() ));
            }catch ( std::exception & ex)
            {
                // it can not use the logger yet, hence, will use the std::err
                std::cerr << "Failed to read the config file " << pathMan.getLogConfFilePath() << ". All settings will be set to their default value" << std::endl;
            }
        }



        LoggerSophosSettings::~LoggerSophosSettings()
        {

        }

        LoggerSophosSettings& LoggerSophosSettings::instance()
        {
            static LoggerSophosSettings loggerSophosSettings;
            return loggerSophosSettings;
        }

        bool LoggerSophosSettings::hasSpecializationFor(const std::string& productName) const
        {
            if ( m_configTree)
            {
                return m_configTree->hasSession(productName);
            }
            return false;
        }

        bool LoggerSophosSettings::logLevel(const std::string& productName, log4cplus::LogLevel& logLevelOut) const
        {
            std::string verbosity;

            if ( hasSpecializationFor(productName))
            {
                // it can assume config tree is valid as there is a specialization for the product
                verbosity = m_configTree->getVerbosity(productName);
            }
            else
            {
                if ( m_configTree)
                {
                    // get the option for global
                    verbosity = m_configTree->getVerbosity();
                }
            }

            if ( verbosity.empty())
            {
                return false;
            }

            return fromNameToLog(verbosity, logLevelOut);
        }


    }
}

