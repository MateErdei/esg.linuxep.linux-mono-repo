
#include "Logger.h"
#include <Common/Logging/FileLoggingSetup.h>
#include <thread>


namespace Comms {
    int main_entry() {
        Common::Logging::FileLoggingSetup loggerSetup("CommsComponent");
        LOGINFO("Started Comms Component");
        while (true) {
            LOGINFO("Comms Component Running");
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
        return 0;
    }
} // namespace Comms