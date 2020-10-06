/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>
#include <vector>

#include <boost/program_options.hpp>
#include <log4cplus/loglevel.h>

namespace po = boost::program_options;

namespace avscanner::avscannerimpl
{
    class Options
    {
    public:
        Options(int argc, char* argv[]);
        Options(bool printHelp, std::vector<std::string> paths, std::vector<std::string> exclusions, bool archiveScanning);
        static log4cplus::LogLevel verifyLogLevel(const std::string& logLevel);

        [[nodiscard]] std::string config() const
        {
            return m_config;
        }

        [[nodiscard]] std::vector<std::string> paths() const
        {
            return m_paths;
        }

        [[nodiscard]] bool archiveScanning() const
        {
            return m_archiveScanning;
        }

        [[nodiscard]] std::vector<std::string> exclusions() const
        {
            return m_exclusions;
        }

        [[nodiscard]] bool help() const
        {
            return m_printHelp;
        }

        static std::string getHelp();

        [[nodiscard]] std::string logFile() const
        {
            return m_logFile;
        }

        [[nodiscard]] log4cplus::LogLevel logLevel() const
        {
            return m_logLevel;
        }

    private:
        std::string m_config;
        std::string m_logFile;

        log4cplus::LogLevel  m_logLevel = log4cplus::NOT_SET_LOG_LEVEL;
        bool m_printHelp = false;
        std::vector <std::string> m_paths;
        std::vector <std::string> m_exclusions;
        bool m_archiveScanning = false;

        inline static std::unique_ptr<po::options_description> m_optionsDescription = nullptr;
        static boost::program_options::variables_map parseCommandLine(int argc, char** argv);
        static void constructOptions();
    };
}

