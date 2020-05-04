/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <stdexcept>
#include "Options.h"

using namespace avscanner::avscannerimpl;
namespace po = boost::program_options;

po::variables_map Options::parseCommandLine(int argc, char** argv)
{

    po::positional_options_description positionalOptions;
    positionalOptions.add("files", -1);

    po::options_description optionalOptions("Allowed options");
    optionalOptions.add_options()
        ("files,f", po::value< std::vector<std::string> >(), "files to scan")
        ("config,c", po::value<std::string>(), "input configuration file for scheduled scans")
        ("scan-archives,s","scan inside archives")
        ("exclude,ex", po::value< std::vector<std::string> >(),"exclude these locations from being scanned")
        ;

    po::variables_map vm;
    store(po::command_line_parser(argc, argv)
                                      .options(optionalOptions)
                                      .positional(positionalOptions)
                                      .run(),
                                   vm);

    return vm;
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
    }
}
