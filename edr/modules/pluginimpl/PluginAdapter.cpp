/******************************************************************************************************

Copyright 2018-2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginAdapter.h"
#include "ApplicationPaths.h"
#include "Logger.h"
#include "TelemetryConsts.h"
#include "IOsqueryProcess.h"

#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <modules/Proc/ProcUtilities.h>

#include <cmath>


    // helper class that allow to schedule a task.
    // but it also has some capability of interrupting the scheduler at any point
    // which is used to respond on shutdown request.
    class WaitUpTo
    {
        std::chrono::milliseconds m_ms;
        std::function<void()> m_onFinish;
        std::promise<void> m_promise;
        std::future<void> m_anotherThread;
        std::atomic<bool> m_finished;

    public:
        WaitUpTo(std::chrono::milliseconds ms, std::function<void(void)> onFinish) :
            m_ms(ms),
            m_onFinish(std::move(onFinish)),
            m_promise {},
            m_anotherThread {},
            m_finished { false }
        {
            m_anotherThread = std::async(std::launch::async, [this]() {
              if (m_promise.get_future().wait_for(m_ms) == std::future_status::timeout)
              {
                  m_finished = true;
                  m_onFinish();
              }
            });
        }
        void cancel()
        {
            if (!m_finished.exchange(true, std::memory_order::memory_order_seq_cst))
            {
                m_promise.set_value();
            }
        }
        ~WaitUpTo()
        {
            cancel();
        }
    };

namespace Plugin
{
    PluginAdapter::PluginAdapter(
        std::shared_ptr<QueueTask> queueTask,
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
        std::shared_ptr<PluginCallback> callback) :
            m_queueTask(std::move(queueTask)),
            m_baseService(std::move(baseService)),
            m_callback(std::move(callback)),
            m_timesOsqueryProcessFailedToStart(0)
    {
    }

    void PluginAdapter::mainLoop()
    {
        LOGINFO("Entering the main loop");
        setUpOsqueryMonitor();

        std::unique_ptr<WaitUpTo> m_delayedRestart;
        while (true)
        {
            Task task = m_queueTask->pop();
            switch (task.taskType)
            {
                case Task::TaskType::STOP:
                    LOGDEBUG("Process task STOP");
                    stopOsquery();
                    return;
                case Task::TaskType::RESTARTOSQUERY:
                    LOGDEBUG("Process task RESTARTOSQUERY");
                    LOGINFO("Restarting osquery");
                    setUpOsqueryMonitor();
                    break;
                case Task::TaskType::OSQUERYPROCESSFINISHED:
                    LOGDEBUG("Process task OSQUERYPROCESSFINISHED");
                    m_timesOsqueryProcessFailedToStart = 0;
                    LOGDEBUG("osquery stopped. Scheduling its restart in 10 seconds.");
                    Common::Telemetry::TelemetryHelper::getInstance().increment(plugin::telemetryOsqueryRestarts, 1UL);
                    m_delayedRestart.reset( // NOLINT
                        new WaitUpTo(std::chrono::seconds(10), [this]() { this->m_queueTask->pushRestartOsquery(); }));
                    break;

                case Task::TaskType::OSQUERYPROCESSFAILEDTOSTART:
                    static unsigned int baseDelay = 10;
                    static unsigned int growthBase = 2;
                    static unsigned int maxTime = 320;

                    auto delay = static_cast<unsigned int>(baseDelay * ::pow(growthBase, m_timesOsqueryProcessFailedToStart));
                    if (delay < maxTime)
                    {
                        m_timesOsqueryProcessFailedToStart++;
                    }
                    LOGDEBUG("The osquery process failed to start. Scheduling a retry in " << delay << " seconds.");
                    Common::Telemetry::TelemetryHelper::getInstance().increment(plugin::telemetryOsqueryRestarts, 1UL);
                    m_delayedRestart.reset( //NOLINT
                        new WaitUpTo(std::chrono::seconds(delay), [this]() { this->m_queueTask->pushRestartOsquery(); }));
            }
        }
    }

    void PluginAdapter::setUpOsqueryMonitor()
    {
        LOGDEBUG("Setup monitoring of osquery");
        std::shared_ptr<QueueTask> queue = m_queueTask;
        std::shared_ptr<Plugin::IOsqueryProcess> osqueryProcess { createOsqueryProcess() };
        m_osqueryProcess = osqueryProcess;
        m_monitor = std::async(std::launch::async, [queue, osqueryProcess]() {
            try
            {
                osqueryProcess->keepOsqueryRunning();
            }
            catch (Plugin::IOsqueryCrashed&)
            {
                LOGWARN("osquery stopped unexpectedly");
            }
            catch (const Plugin::IOsqueryCannotBeExecuted& exception)
            {
                LOGERROR("Unable to execute osquery: " << exception.what());
                queue->pushOsqueryProcessDelayRestart();
                return;
            }
            catch (std::exception& ex)
            {
                LOGERROR("Monitor process failed: " << ex.what());
            }
            queue->pushOsqueryProcessFinished();
        });
    }

    void PluginAdapter::stopOsquery()
    {
        try
        {
            while(m_osqueryProcess && m_monitor.valid())
            {
                LOGINFO("Issue request to stop to osquery.");
                m_osqueryProcess->requestStop();

                if(m_monitor.wait_for(std::chrono::seconds(2)) == std::future_status::timeout)
                {
                    LOGWARN("Timeout while waiting for osquery monitoring to finish.");
                    continue;
                }
                m_monitor.get(); // ensure that IOsqueryProcess::keepOsqueryRunning has finished.
            }
        }
        catch (std::exception & ex)
        {
            LOGWARN("Unexpected exception: " << ex.what());
        }
    }

    PluginAdapter::~PluginAdapter()
    {
        try
        {
            if (m_monitor.valid())
            {
                stopOsquery();       // in case it reach this point without the request to stop being issued.
            }
        }catch (std::exception& ex)
        {
            std::cerr << "Plugin adapter exception: " << ex.what() << std::endl;
        }
        // safe to clean up.
    }
} // namespace Plugin
