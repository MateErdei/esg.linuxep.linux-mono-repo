/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "sync_versioned_files.h"

#include "datatypes/Print.h"

#include <boost/program_options.hpp>

namespace po = boost::program_options;

int main(int argc, char* argv[])
{
    po::positional_options_description positionalOptions;
    positionalOptions.add("source", 1);
    positionalOptions.add("destination", 2);

    po::options_description optionalOptions("Allowed options");
    optionalOptions.add_options()
        ("source", po::value<std::string>(), "Source directory to sync from")
        ("destination", po::value<std::string>(), "Destination directory to sync files to")
        ("help", "produce help message")
        ("notVersioned", "sync non-versioned files")
        ("copy", "copy new/changed files, and delete removed files")
        ;

    po::variables_map variableMap;

    po::store(po::command_line_parser(argc, argv)
              .positional(positionalOptions)
              .options(optionalOptions)
              .run(),
              variableMap);

    if (variableMap.count("help"))
    {
        PRINT("Syntax: sync_versioned_files <src> <dest> [--notVersioned]");
        return 2;
    }

    bool isVersioned = true;
    if (variableMap.count("notVersioned"))
    {
        isVersioned = false;
    }

    auto source = variableMap["source"].as<std::string>();
    auto destination = variableMap["destination"].as<std::string>();

    bool copy = (variableMap.count("copy") > 0);

    if (copy)
    {
        return sync_versioned_files::copy(source, destination);
    }
    else
    {
        return sync_versioned_files::sync_versioned_files(source, destination, isVersioned);
    }
}
