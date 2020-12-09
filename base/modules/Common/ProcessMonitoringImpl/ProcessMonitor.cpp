/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ProcessMonitor.h"

#include "Logger.h"

#include "Common/ProcessMonitoringImpl/SignalHandler.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/FileSystem/IFilePermissions.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/PluginRegistryImpl/PluginInfo.h>
#include <Common/Threads/NotifyPipe.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <Common/ZMQWrapperApi/IContext.h>
#include <Common/ZeroMQWrapper/IPoller.h>
#include <Common/ZeroMQWrapper/ISocketReplier.h>
#include <sys/select.h>
#include <sys/stat.h>

#include <cassert>
#include <cstdlib>
#include <unistd.h>

namespace Common
{
    namespace ProcessMonitoringImpl
    {
        ProcessMonitor::ProcessMonitor() : ProcessMonitor(Common::ZMQWrapperApi::createContext()) {}

        ProcessMonitor::ProcessMonitor(Common::ZMQWrapperApi::IContextSharedPtr context) : m_context(std::move(context))
        {
        }

        ProcessMonitor::~ProcessMonitor() { m_context.reset(); }

        int ProcessMonitor::run()
        {
            Common::ProcessMonitoringImpl::SignalHandler signalHandler;

            {
                std::lock_guard<std::mutex> lock(m_processProxiesMutex);
                if (m_processProxies.empty())
                {
                    LOGERROR("No processes to monitor!");
                    return 1;
                }

                for (auto& proxy : m_processProxies)
                {
                    proxy->ensureStateMatchesOptions();
                }
            }

            bool keepRunning = true;

            Common::ZeroMQWrapper::IPollerPtr poller = Common::ZeroMQWrapper::createPoller();

            Common::ZeroMQWrapper::IHasFDPtr subprocessFD = poller->addEntry(
                signalHandler.subprocessExitFileDescriptor(), Common::ZeroMQWrapper::IPoller::PollDirection::POLLIN);
            Common::ZeroMQWrapper::IHasFDPtr terminationFD = poller->addEntry(
                signalHandler.terminationFileDescriptor(), Common::ZeroMQWrapper::IPoller::PollDirection::POLLIN);

            for (auto& socketHandleFunction : m_socketHandleFunctionList)
            {
                poller->addEntry(*socketHandleFunction.first, Common::ZeroMQWrapper::IPoller::PollDirection::POLLIN);
            }

            std::chrono::seconds timeout(10);

            Common::UtilityImpl::FormattedTime time;
            while (keepRunning)
            {
                LOGDEBUG("Calling poller at " << time.currentTime());
                Common::ZeroMQWrapper::IPoller::poll_result_t active = poller->poll(std::chrono::milliseconds(timeout));
                LOGDEBUG("Returned from poller: " << active.size() << " at " << ::time(nullptr));

                for (auto& fd : active)
                {
                    if (fd == terminationFD.get())
                    {
                        LOGWARN("Process Monitoring Exiting");
                        signalHandler.clearTerminationPipe();
                        keepRunning = false;
                        continue;
                    }
                    if (fd == subprocessFD.get())
                    {
                        // Don't log since we don't know if the exit was expected
                        signalHandler.clearSubProcessExitPipe();
                    }
                    for (auto& socketHandleFunction : m_socketHandleFunctionList)
                    {
                        if (fd == socketHandleFunction.first)
                        {
                            try
                            {
                                socketHandleFunction.second();
                            }
                            catch (const std::exception& exception)
                            {
                                LOGERROR(
                                    "Unexpected error occurred when handling socket communication: "
                                    << exception.what());
                            }
                            catch (...)
                            {
                                LOGERROR("Unknown error occurred when handling socket communication.");
                            }
                        }
                    }
                }

                // Child may have exited, or socket request may have altered state.
                timeout = std::chrono::seconds(10);
                {
                    std::lock_guard<std::mutex> lock(m_processProxiesMutex);
                    for (auto& proxy : m_processProxies)
                    {
                        auto waitPeriod = proxy->checkForExit().first;
                        waitPeriod = std::min(proxy->ensureStateMatchesOptions(), waitPeriod);
                        timeout = std::min(waitPeriod, timeout);
                    }
                }
                timeout = std::max(timeout, std::chrono::seconds(1)); // Ensure we wait at least 1 second
                LOGDEBUG("timeout = " << timeout.count());
            }

            LOGINFO("Stopping processes");
            {
                std::lock_guard<std::mutex> lock(m_processProxiesMutex);
                m_processProxies.clear(); // stop is called on proxies as they are destroyed
            }
            // Normal shutdown
            m_socketHandleFunctionList.clear();
            m_context.reset();

            return 0;
        }

        void ProcessMonitor::addProcessToMonitor(Common::ProcessMonitoring::IProcessProxyPtr processProxyPtr)
        {
            std::lock_guard<std::mutex> lock(m_processProxiesMutex);
            m_processProxies.push_back(std::move(processProxyPtr));
        }

        void ProcessMonitor::addReplierSocketAndHandleToPoll(
            Common::ZeroMQWrapper::ISocketReplier* socketReplier,
            std::function<void(void)> socketHandleFunction)
        {
            m_socketHandleFunctionList.push_back(SocketHandleFunctionPair(socketReplier, socketHandleFunction));
        }

        std::vector<std::string> ProcessMonitor::getListOfPluginNames()
        {
            std::lock_guard<std::mutex> lock(m_processProxiesMutex);
            std::vector<std::string> pluginNames;
            for (auto& processProxy : m_processProxies)
            {
                std::string name = processProxy->name();
                assert(processProxy.get() != nullptr);
                if (!name.empty())
                {
                    pluginNames.emplace_back(name);
                }
            }
            return pluginNames;
        }

        bool ProcessMonitor::removePluginByName(const std::string& pluginName)
        {
            if (pluginName.empty())
            {
                return false;
            }
            bool found = false;
            std::lock_guard<std::mutex> lock(m_processProxiesMutex);
            for (auto it = m_processProxies.begin(); it != m_processProxies.end();)
            {
                Common::ProcessMonitoring::IProcessProxy* pluginProxy = it->get();
                if (pluginProxy == nullptr) // necessary for release build.
                {
                    break;
                }
                if (pluginName == pluginProxy->name())
                {
                    pluginProxy->stop();
                    it = m_processProxies.erase(it);
                    found = true;
                }
                else
                {
                    ++it;
                }
            }

            return found;
        }

        void ProcessMonitor::applyToProcessProxy(
            const std::string& processProxyName,
            std::function<void(Common::ProcessMonitoring::IProcessProxy&)> functorToApply)
        {
            std::lock_guard<std::mutex> lock(m_processProxiesMutex);
            for (auto& proxy : m_processProxies)
            {
                assert(proxy.get() != nullptr);
                if (proxy.get() == nullptr)
                {
                    continue;
                }
                if (proxy->name() == processProxyName)
                {
                    functorToApply(*proxy);
                }
            }
        }

    } // namespace ProcessMonitoringImpl
} // namespace Common

namespace Common::ProcessMonitoring
{
    IProcessMonitorPtr createProcessMonitor()
    {
        return IProcessMonitorPtr(new Common::ProcessMonitoringImpl::ProcessMonitor());
    }
} // namespace Common::ProcessMonitoring