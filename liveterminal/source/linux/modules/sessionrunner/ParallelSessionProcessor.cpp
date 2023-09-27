/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystem/IFileSystem.h>
#include "ParallelSessionProcessor.h"
#include "Logger.h"
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <modules/pluginimpl/Telemetry/TelemetryConsts.h>

namespace sessionrunner{

    ParallelSessionProcessor::ParallelSessionProcessor(std::unique_ptr<sessionrunner::ISessionRunner> sessionProcessor, const std::string& sessionsDirectoryPath):
            m_sessionProcessor{std::move(sessionProcessor)}, m_sessionsDirectoryPath{sessionsDirectoryPath}
    {
    }

    ParallelSessionProcessor::~ParallelSessionProcessor()
    {
        try
        {
            {
                std::lock_guard<std::mutex> l{m_mutex};
                for (auto& p: m_runningSessions)
                {
                    p->requestAbort(); 
                }
            }
            // give up to 0.5 seconds to have all the sessions processed.
            int count = 0;
            while (count++ < 50)
            {
                if (m_runningSessions.empty())
                {
                    break;
                }
                {
                    std::lock_guard<std::mutex> l{m_mutex};
                    for (auto& p: m_runningSessions)
                    {
                        p->requestAbort(); 
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10)); 
            }
                    
            {
                std::lock_guard<std::mutex> l{m_mutex};
                if (!m_runningSessions.empty())
                {
                    LOGERROR("Should not have any further sessions to process.");
                }
                m_processedSessions.clear();
            }
        }
        catch (std::exception& ex)
        {
            LOGERROR("Failure in clean up ParallelSessionProcessor: " << ex.what());
        }        
    }

    void ParallelSessionProcessor::jobDoneCallback(const std::string& id)
    {
        try
        {
            std::lock_guard<std::mutex> l{ m_mutex };
            LOGINFO("Session " << id << " finished");
            // clearing up previous session objects that now can be removed as they are not in the same thread.
            m_processedSessions.clear();
            auto it = std::find_if(
                m_runningSessions.begin(),
                m_runningSessions.end(),
                [id](const std::unique_ptr<sessionrunner::ISessionRunner>& irunner) { return irunner->id() == id; });

            if (it != m_runningSessions.end())
            {
                LOGDEBUG("Removing closed session from list of running sessions");
                // move this element to the other list to clear it afterwards.
                // it cannot be clear here as this is executed in the callback of the sessionrunnerthread.
                m_processedSessions.splice(m_processedSessions.begin(), m_runningSessions, it);
            }
            else
            {
                LOGWARN("Failed to find the session " << id << " in the list of running sessions");
            }

            auto fileSystem = Common::FileSystem::fileSystem();
            std::string filepath = Common::FileSystem::join(m_sessionsDirectoryPath, id);
            if (fileSystem->isFile(filepath))
            {
                fileSystem->removeFile(filepath);
            }
        }
        catch (std::exception& ex)
        {
            LOGERROR("Failed to handle callback on session finished: " << ex.what());
        }
    }

    void ParallelSessionProcessor::addJob(const std::string &id)
    {
        try
        {
            std::lock_guard<std::mutex> l{ m_mutex };
            auto it = std::find_if(
                m_runningSessions.begin(),
                m_runningSessions.end(),
                [id](const std::unique_ptr<sessionrunner::ISessionRunner>& irunner) { return irunner->id() == id; });

            if (it != m_runningSessions.end())
            {
                LOGERROR("Session id " << id << " already running. Will not trigger duplicate");
                Common::Telemetry::TelemetryHelper::getInstance().increment(Plugin::Telemetry::failedSessionsTag, 1L);
                return;
            }

            auto newProcess = m_sessionProcessor->clone();
            newProcess->triggerSession(id, [this](std::string id) { this->jobDoneCallback(id); });
            m_runningSessions.emplace_back(std::move(newProcess));
        }
        catch (std::exception &ex)
        {
            Common::Telemetry::TelemetryHelper::getInstance().increment(Plugin::Telemetry::failedSessionsTag, 1L);
            LOGERROR("Failed to configure a new session: " << ex.what());
        }
    }
}
