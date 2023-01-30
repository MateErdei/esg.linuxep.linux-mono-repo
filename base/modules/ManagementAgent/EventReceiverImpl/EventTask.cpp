// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "EventTask.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <ManagementAgent/LoggerImpl/Logger.h>

#include <cassert>
#include <chrono>
#include <sstream>
#include <sys/stat.h>

ManagementAgent::EventReceiverImpl::EventTask::EventTask(std::string appId, std::string eventXml,
                                                         IOutbreakModeControllerPtr outbreakModeController) :
    m_appId(std::move(appId)),
    m_eventXml(std::move(eventXml)),
    outbreakModeController_(std::move(outbreakModeController))
{
}

namespace
{
    std::string createTimestamp()
    {
        auto nowTime = std::chrono::high_resolution_clock::now();

        std::ostringstream ost;
        ost << std::chrono::duration_cast<std::chrono::microseconds>(nowTime.time_since_epoch()).count();

        return ost.str();
    }

    Path createEventBasename(const std::string& appId)
    {
        std::string nonce = createTimestamp();
        return appId + "_event-" + nonce + ".xml";
    }
} // namespace

void ManagementAgent::EventReceiverImpl::EventTask::run()
{
    // Determine if event should be filtered by Outbreak Mode
    if (
        outbreakModeController_->recordEventAndDetermineIfItShouldBeDropped(m_appId, m_eventXml)
    )
    {
        // Drop the event
        return;
    }

    LOGSUPPORT("Send event from appid " << m_appId << " to mcsrouter");
    Path eventDir = Common::ApplicationConfiguration::applicationPathManager().getMcsEventFilePath();
    assert(!eventDir.empty());
    Path tmpDir = Common::ApplicationConfiguration::applicationPathManager().getTempPath();
    assert(!tmpDir.empty());

    Path basename = createEventBasename(m_appId);
    assert(!basename.empty());

    Path dest = Common::FileSystem::join(eventDir, basename);
    assert(!dest.empty());

    Common::FileSystem::fileSystem()->writeFileAtomically(dest, m_eventXml, tmpDir, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
}
