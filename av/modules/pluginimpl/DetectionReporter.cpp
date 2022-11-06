// Copyright 2022, Sophos Limited.  All rights reserved.

#include "DetectionReporter.h"

#include "Logger.h"

namespace Plugin
{
    void DetectionReporter::processThreatReport(
        const std::string& threatDetectedXML,
        const std::shared_ptr<TaskQueue>& taskQueue)
    {
        LOGDEBUG("Sending threat detection notification to Central: " << threatDetectedXML);
        taskQueue->push(Task { .taskType = Task::TaskType::ThreatDetected, .Content = threatDetectedXML });
    }

    void DetectionReporter::publishQuarantineCleanEvent(
        const std::string& coreCleanEventXml,
        const std::shared_ptr<TaskQueue>& taskQueue)
    {
        LOGDEBUG("Sending Clean Event to Central: " << coreCleanEventXml);
        taskQueue->push(Task { .taskType = Task::TaskType::SendCleanEvent, .Content = coreCleanEventXml });
    }
} // namespace Plugin
