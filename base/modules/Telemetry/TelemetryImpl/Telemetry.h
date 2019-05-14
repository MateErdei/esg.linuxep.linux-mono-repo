/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "TelemetryProcessor.h"

#include <Common/HttpSenderImpl/HttpSender.h>

namespace Telemetry
{
    int main(Common::HttpSender::IHttpSender& httpSender, TelemetryProcessor& telemetryProcessor);

    /**
     * To be used when parsing arguments from argv as received in int main( int argc, char * argv[]).
     *
     * It logs a message then returns.
     *
     * @param argc As convention the number of valid entries in argv with the program as the first argument.
     * @param argv As convention an array of string arguments.
     * @return
     */
    int main_entry(int argc, char* argv[]);
} // namespace Telemetry
