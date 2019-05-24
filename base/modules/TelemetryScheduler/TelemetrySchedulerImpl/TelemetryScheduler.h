/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

namespace TelemetrySchedulerImpl
{
    /**
     * To be used when parsing arguments from argv as received in int main( int argc, char * argv[]).
     *
     * Logs a message then returns.
     *
     * @param argc As convention the number of valid entries in argv with the program as the first argument.
     * @param argv As convention an array of string arguments.
     * @return
     */
    int main_entry();
} // namespace Telemetry
