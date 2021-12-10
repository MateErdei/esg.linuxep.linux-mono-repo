/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <stdexcept>
#include "Options.h"
#include "Logger.h"
#include <boost/algorithm/string.hpp>
using namespace avscanner::avscannerimpl;
namespace po = boost::program_options;

log4cplus::LogLevel Options::verifyLogLevel(const std::string& logLevel)
{
    std::string lowerCaseLogLevel(boost::to_lower_copy(logLevel));

    if(lowerCaseLogLevel == "debug")
        return log4cplus::DEBUG_LOG_LEVEL;
    if(lowerCaseLogLevel == "support")
        return log4cplus::SUPPORT_LOG_LEVEL;
    if(lowerCaseLogLevel == "info")
        return log4cplus::INFO_LOG_LEVEL;
    if(lowerCaseLogLevel == "warn")
        return log4cplus::WARN_LOG_LEVEL;
    if(lowerCaseLogLevel == "error")
        return log4cplus::ERROR_LOG_LEVEL;
    if(lowerCaseLogLevel == "off")
        return log4cplus::OFF_LOG_LEVEL;

    throw po::error(std::string("Unrecognised Log Level"));
}

po::variables_map Options::parseCommandLine(int argc, char** argv)
{
    Options::constructOptions();
    po::positional_options_description positionalOptions;
    positionalOptions.add("files", -1);

    po::variables_map vm;
    store(po::command_line_parser(argc, argv)
              .positional(positionalOptions)
              .options(*m_optionsDescription)
              .run(),
          vm);

    return vm;
}

void Options::constructOptions()
{
    if(!m_optionsDescription)
    {
        m_optionsDescription = std::make_unique<po::options_description>("Allowed options");
        m_optionsDescription->add_options()
            ("help,h", "Print this help message")
            ("scan-archives,a", "Scan inside archives")
            ("scan-images,i", "Scan inside images")
            ("follow-symlinks,s", "Follow symlinks while scanning")
            ("exclude,x",po::value<std::vector<std::string>>()->value_name("EXCLUSION [EXCLUSION...]")->multitoken(),"Exclude these locations from being scanned")
            ("output,o", po::value<std::string>()->value_name("OUTPUT"), "Write to log file")
            ("log-level,l", po::value<std::string>()->value_name("LOGLEVEL"), "Log level for Command Line Scanner")
            ("files,f", po::value<std::vector<std::string>>()->value_name("file [file...]")->multitoken(), "Files to scan")
            ("config,c", po::value<std::string>()->value_name("config_file"), "Input configuration file for scheduled scans");
    }
}

Options::Options(int argc, char** argv)
{
    if (argc > 0)
    {
        auto variableMap = parseCommandLine(argc, argv);

        if (variableMap.count("files"))
        {
            m_paths = variableMap["files"].as<std::vector<std::string>>();
        }

        if (variableMap.count("scan-archives"))
        {
            m_archiveScanning = true;
        }

        if (variableMap.count("scan-images"))
        {
            m_imageScanning = true;
        }

        if (variableMap.count("follow-symlinks"))
        {
            m_followSymlinks = true;
        }

        if (variableMap.count("exclude"))
        {
            m_exclusions = variableMap["exclude"].as<std::vector<std::string>>();
        }

        if (variableMap.count("config"))
        {
            m_config = variableMap["config"].as<std::string>();
        }

        if (variableMap.count("output"))
        {
            m_logFile = variableMap["output"].as<std::string>();
        }

        if (variableMap.count("log-level"))
        {
            m_logLevel = verifyLogLevel(variableMap["log-level"].as<std::string>());
        }

        if (variableMap.count("help"))
        {
            m_printHelp = true;
        }
    }
}

Options::Options(bool printHelp, std::vector<std::string> paths, std::vector<std::string> exclusions, bool archiveScanning, bool imageScanning, bool followSymlinks)
    : m_printHelp(printHelp),
    m_paths(std::move(paths)),
    m_exclusions(std::move(exclusions)),
    m_archiveScanning(archiveScanning),
    m_imageScanning(imageScanning),
    m_followSymlinks(followSymlinks)
{
    constructOptions();
}

std::string Options::getHelp()
{
    std::ostringstream helpText;
    helpText << "Usage: avscanner PATH... [OPTION]..." << std::endl;
    helpText << "Perform an on-demand scan of PATH" << std::endl << std::endl;
    helpText << "Allowed options:" << std::endl;
    helpText << "  -h, --help                              Print this help message" << std::endl;
    helpText << "  -a, --scan-archives                     Scan inside archives" << std::endl;
    helpText << "  -i, --scan-images                       Scan inside images" << std::endl;
    helpText << "  -s, --follow-symlinks                   Follow symlinks while scanning" << std::endl;
    helpText << "  -x, --exclude EXCLUSION [EXCLUSION...]  Exclude these locations from being scanned" << std::endl;
    helpText << "  -o, --output OUTPUT                     Write to log file" << std::endl;
    helpText << "  -l, --log-level LOGLEVEL                Set the logging level" << std::endl << std::endl;

    helpText << "Examples:" << std::endl;
    helpText << "  avscanner / --scan-archives            Scan the Root Directory (recursively including dot files/directories) including the contents of any archive files found" << std::endl;
    helpText << "  avscanner / --scan-images              Scan the Root Directory including contents of images" << std::endl;
    helpText << "  avscanner / --follow-symlinks          Scan the Root Directory and follow any symlinks encountered" << std::endl;
    helpText << "  avscanner /usr --exclude /usr/local/   Scan the /usr directory excluding /usr/local" << std::endl;
    helpText << "  avscanner folder --exclude '*.log'     Scan the directory named 'folder' but exclude any filenames ending with .log" << std::endl;
    helpText << "  avscanner foo.exe -o scan.log          Scan the file 'foo.exe' and redirect the output to a log file named 'scan.log'" << std::endl;
    helpText << "  avscanner / --log-level info           Scan the Root Directory with log level set to INFO" << std::endl;
    return helpText.str();
}
