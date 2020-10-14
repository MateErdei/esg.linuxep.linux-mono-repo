/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "avscanner/avscannerimpl/Options.h"

#include <avscanner/avscannerimpl/CommandLineScanRunner.h>
#include <avscanner/avscannerimpl/NamedScanRunner.h>
#include <datatypes/Print.h>
#include <datatypes/sophos_filesystem.h>

#include <memory>

using namespace avscanner::avscannerimpl;
namespace fs = sophos_filesystem;


int main(int argc, char* argv[])
{
    enum E_ERROR_CODES: int
    {
        E_CLEAN_EXIT,
        E_GENERAL_ERROR,
        E_UNKNOWN_OPTION,
        E_BAD_OPTION
    };

    try
    {
        Options options(argc, argv);

        auto config = options.config();

        if(options.help())
        {
            PRINT(Options::getHelp());
            return E_CLEAN_EXIT;
        }

        if (fs::is_directory(options.logFile()))
        {
            PRINT(Options::getHelp());
            PRINT("Failed to log to " << options.logFile() << " as it is a directory");
            return E_BAD_OPTION;
        }

        std::unique_ptr<IRunner> runner;
        if (config.empty())
        {
            runner = std::make_unique<CommandLineScanRunner>(options);
        }
        else
        {
            runner = std::make_unique<NamedScanRunner>(config);
        }

        return runner->run();
    }
    catch (const boost::program_options::unknown_option& e)
    {
        PRINT(Options::getHelp());
        PRINT("Unrecognised option: " << e.get_option_name());

        return E_UNKNOWN_OPTION;
    }
    catch (const boost::program_options::error& e)
    {
        PRINT(Options::getHelp());
        PRINT("Failed to parse command line options: " << e.what());

        return E_BAD_OPTION;
    }
    catch (const std::exception& e)
    {
        PRINT("Command Line Scanner failed: " << e.what());

        return E_GENERAL_ERROR;
    }
}
