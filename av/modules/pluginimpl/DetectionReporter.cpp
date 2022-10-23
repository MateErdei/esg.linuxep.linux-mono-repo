// Copyright 2022, Sophos Limited.  All rights reserved.

#include "DetectionReporter.h"

#include "Logger.h"

#include "Common/XmlUtilities/AttributesMap.h"

namespace Plugin
{
    void DetectionReporter::processThreatReport(const std::string& threatDetectedXML, const std::shared_ptr<TaskQueue>& taskQueue)
    {
        LOGDEBUG("Sending threat detection notification to central: " << threatDetectedXML);
        taskQueue->push(Task { .taskType = Task::TaskType::ThreatDetected, .Content = threatDetectedXML });
    }
}
