// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "Event.h"

#include "ManagementAgent/LoggerImpl/Logger.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileSystem.h"

#include <chrono>
#include <utility>

#include <sys/stat.h>

using namespace ManagementAgent::EventReceiverImpl;

Event::Event(std::string appId, std::string eventXml)
    : appId_(std::move(appId)), eventXml_(std::move(eventXml))
{}

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

void Event::send() const
{
    LOGDEBUG("Send event from appid " << appId_ << " to mcsrouter");
    Path eventDir = Common::ApplicationConfiguration::applicationPathManager().getMcsEventFilePath();
    assert(!eventDir.empty());
    Path tmpDir = Common::ApplicationConfiguration::applicationPathManager().getTempPath();
    assert(!tmpDir.empty());

    Path basename = createEventBasename(appId_);
    assert(!basename.empty());

    Path dest = Common::FileSystem::join(eventDir, basename);
    assert(!dest.empty());

    Common::FileSystem::fileSystem()->writeFileAtomically(dest, eventXml_, tmpDir, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
}
