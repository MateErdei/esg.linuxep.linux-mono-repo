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
    };
}
