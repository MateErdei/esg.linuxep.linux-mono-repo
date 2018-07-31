/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "EventTask.h"
#include "Logger.h"
#include "Common/FileSystem/IFileSystem.h"

#include <chrono>
#include <cassert>
#include <sstream>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>


ManagementAgent::EventReceiverImpl::EventTask::EventTask(std::string appId, std::string eventXml)
    :
        m_appId(std::move(appId)),
        m_eventXml(std::move(eventXml))
{

}


namespace
{
    std::string createTimestamp()
    {
        auto nowTime = std::chrono::high_resolution_clock::now();


        std::ostringstream ost;
        ost << std::chrono::duration_cast<std::chrono::microseconds>
                (nowTime.time_since_epoch()).count();

        return ost.str();
    }

    Path createEventBasename(
            const std::string& appId
        )
    {
        std::string nonce = createTimestamp();
        return appId+"_event-"+nonce+".xml";
    }
}

void ManagementAgent::EventReceiverImpl::EventTask::run()
{
    LOGSUPPORT("Send event from appid " << m_appId << " to mcsrouter");
    Path eventDir = Common::ApplicationConfiguration::applicationPathManager().getMcsEventFilePath();
    assert(!eventDir.empty());
    Path tmpDir = Common::ApplicationConfiguration::applicationPathManager().getTempPath();
    assert(!tmpDir.empty());

    Path basename = createEventBasename(m_appId);
    assert(!basename.empty());

    Path dest = Common::FileSystem::fileSystem()->join(eventDir,basename);
    assert(!dest.empty());

    Common::FileSystem::fileSystem()->writeFileAtomically(dest, m_eventXml, tmpDir);
}


