/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

namespace Telemetry
{
    /**
     * To be used when parsing arguments from argv as received in int main( int argc, char * argv[]).
     *
     * It runs fileEntriesAndRunDownloader and forward its return.
     *
     * It accepts only the following usage:
     * argv => SulDownloader <InputPath> <OutputPath>
     * Hence, it require argc == 3 and passes the InputPath and OutputPath to fileEntriesAndRunDownloader.
     *
     * @param argc As convention, the number of valid entries in argv with the program as the first argument.
     * @param argv As convention the strings of arguments.
     * @return
     */
    int main_entry(int argc, char* argv[]);
} // namespace Telemetry
