/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <stdexcept>
#include "Options.h"
#include "Logger.h"

using namespace avscanner::avscannerimpl;
namespace po = boost::program_options;

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
            ("files,f", po::value<std::vector<std::string>>()->multitoken(), "Files to scan")
            ("config,c", po::value<std::string>(), "Input configuration file for scheduled scans")
            ("scan-archives,s", "Scan inside archives")
            ("exclude,x",po::value<std::vector<std::string>>()->multitoken(),"Exclude these locations from being scanned");
    }
}

Options::Options(int argc, char** argv)
{
    if (argc > 0)
    {
        auto variableMap = parseCommandLine(argc, argv);

        m_paths.reserve(argc);

        if (variableMap.count("files"))
        {
            m_paths = variableMap["files"].as<std::vector<std::string>>();
        }

        if (variableMap.count("scan-archives"))
        {
            m_archiveScanning = true;
        }

        if (variableMap.count("exclude"))
        {
            m_exclusions = variableMap["exclude"].as<std::vector<std::string>>();
        }

        if (variableMap.count("config"))
        {
            m_config = variableMap["config"].as<std::string>();
        }

        if (variableMap.count("help"))
        {
            m_printHelp = true;
        }
    }
}

Options::Options(bool printHelp, std::vector<std::string> paths, std::vector<std::string> exclusions, bool archiveScanning)
    : m_printHelp(printHelp),
    m_paths(std::move(paths)),
    m_exclusions(std::move(exclusions)),
    m_archiveScanning(archiveScanning)
{
    constructOptions();
}
