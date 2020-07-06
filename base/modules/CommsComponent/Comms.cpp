/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Logger.h"
#include <Common/Logging/FileLoggingSetup.h>
#include <thread>

Common::Logging::FileLoggingSetup loggerSetup("comms_component");
namespace CommsComponent {
    int main_entry() {
        LOGINFO("Started Comms Component");
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
        return 0;
    }
} // namespace CommsComponent