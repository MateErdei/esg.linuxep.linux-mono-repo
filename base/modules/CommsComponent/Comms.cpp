/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Logger.h"
#include <Common/Logging/FileLoggingSetup.h>
#include <thread>
#include <csignal>

namespace
{
    void s_signal_handler(int)
    {
        LOGSUPPORT("Handling signal");
    }

} // namespace

bool stop = false;

namespace CommsComponent {
    int main_entry() {
        Common::Logging::FileLoggingSetup loggerSetup("comms_component");
        LOGINFO("Started Comms Component");
        while (!stop) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            struct sigaction action;
            action.sa_handler = s_signal_handler;
            action.sa_flags = 0;
            sigemptyset(&action.sa_mask);
            sigaction(SIGINT, &action, NULL);
            sigaction(SIGTERM, &action, NULL);
        }
        return 0;
    }
} // namespace CommsComponent