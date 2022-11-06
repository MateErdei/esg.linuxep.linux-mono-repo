// Copyright 2022, Sophos Limited.  All rights reserved.

#include "TaskQueue.h"

#include <memory>
#include <string>

namespace Plugin
{
    class DetectionReporter
    {
    public:
        static void processThreatReport(const std::string&, const std::shared_ptr<TaskQueue>&);

        /*
         * Puts a CORE Clean event task onto the main AV plugin task queue to be sent to Central.
         * XML format can be seen here:
         * https://sophos.atlassian.net/wiki/spaces/SophosCloud/pages/42255827359/EMP+event-core-clean
         */
        static void publishQuarantineCleanEvent(const std::string&, const std::shared_ptr<TaskQueue>&);
    };
} // namespace Plugin
