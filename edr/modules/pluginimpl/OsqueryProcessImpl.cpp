// Copyright 2019-2023 Sophos Limited. All rights reserved.
#include "OsqueryProcessImpl.h"

#include "EdrCommon/ApplicationPaths.h"
#include "Logger.h"
#include "OsqueryLogIngest.h"
#include "PluginAdapter.h"
#include "PluginUtils.h"
#include "EdrCommon/TelemetryConsts.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "Common/ProcUtilImpl/ProcUtilities.h"

#include <iterator>
#include <unistd.h>

namespace
{
    constexpr int OUTPUT_BUFFER_LIMIT_BYTES = 4096;
    constexpr int SECONDS_UNTIL_OSQUERYD_SIGKILL = 5;

    class AnnounceStartedOnEndOfScope
    {
        Plugin::OsqueryStarted& m_osqueryStarted;

    public:
        explicit AnnounceStartedOnEndOfScope(Plugin::OsqueryStarted& osqueryStarted) : m_osqueryStarted(osqueryStarted)
        {
        }
        ~AnnounceStartedOnEndOfScope()
        {
            m_osqueryStarted.announce_started();
        }
    };

    class OsqueryProcessFactory
    {
        OsqueryProcessFactory()
        {
            restoreCreator();
        }

    public:
        static OsqueryProcessFactory& instance()
        {
            static OsqueryProcessFactory osqueryProcessFactory;
            return osqueryProcessFactory;
        }
        void replaceCreator(std::function<Plugin::IOsqueryProcessPtr()> newCreator)
        {
            m_creator = newCreator;
        }
        void restoreCreator()
        {
            m_creator = []() { return Plugin::IOsqueryProcessPtr { new Plugin::OsqueryProcessImpl() }; };
        }
        Plugin::IOsqueryProcessPtr createOsquery()
        {
            return m_creator();
        }
        std::function<Plugin::IOsqueryProcessPtr()> m_creator;
    };

} // namespace

Plugin::IOsqueryProcessPtr Plugin::createOsqueryProcess()
{
    return OsqueryProcessFactory::instance().createOsquery();
}

namespace Plugin
{
    ScopedReplaceOsqueryProcessCreator::ScopedReplaceOsqueryProcessCreator(std::function<IOsqueryProcessPtr()> creator)
    {
        OsqueryProcessFactory::instance().replaceCreator(creator);
    }

    ScopedReplaceOsqueryProcessCreator::~ScopedReplaceOsqueryProcessCreator()
    {
        OsqueryProcessFactory::instance().restoreCreator();
    }

    void OsqueryProcessImpl::keepOsqueryRunning(OsqueryStarted& osqueryStarted)
    {
        std::string osqueryPath;
        {
            AnnounceStartedOnEndOfScope announceStartedOnEndOfScope(osqueryStarted);
            auto fileSystem = Common::FileSystem::fileSystem();
            osqueryPath = Plugin::osqueryPath();
            killAnyOtherOsquery();
            if (!fileSystem->isFile(osqueryPath))
            {
                std::string errorMessage = "Executable does not exist at: " + osqueryPath;
                throw Plugin::IOsqueryCannotBeExecuted(errorMessage);
            }
            LOGINFO("Run osquery process");
            std::string osquerySocket = Plugin::osquerySocket();

            if (fileSystem->exists(osquerySocket))
            {
                fileSystem->removeFileOrDirectory(osquerySocket);
            }

            std::vector<std::string> arguments = { "--config_path=" + Plugin::osqueryConfigFilePath(),
                                                   "--flagfile=" + Plugin::osqueryFlagsFilePath() };

            startProcess(osqueryPath, arguments);
            // only allow commands to be processed after this point.
            // this will trigger osquery_started

            auto fs = Common::FileSystem::fileSystem();
            int osquerySocketWaitLoopCount = 0;
            const int  osquerySocketWaitMaxLoopCount = 600;  //0.1 of a second * 600 = 60 seconds.
            while(!fs->exists(Plugin::osquerySocket()) && osquerySocketWaitLoopCount < osquerySocketWaitMaxLoopCount)
            {
                usleep(100000); // 0.1 of a second
                osquerySocketWaitLoopCount++;
            }
            if (osquerySocketWaitLoopCount >= osquerySocketWaitMaxLoopCount)
            {
                LOGWARN("Timed out waiting for osquery socket file to be created");
            }
        }

        m_processMonitorPtr->waitUntilProcessEnds();
        LOGINFO("The osquery process finished");
        int exitcode = m_processMonitorPtr->exitCode();
        std::string output = m_processMonitorPtr->output();
        LOGDEBUG("The osquery process output and exit code retrieved");
        {
            std::lock_guard lock { m_processMonitorSharedResource };
            try
            {
                m_processMonitorPtr.reset();
            }
            catch (std::exception& ex)
            {
                LOGWARN("Exception on destroying processmonitor " << ex.what());
            }
        }
        // output should be empty. In case the output has only a few white spaces or new lines or even so few characters
        // that it would not give any help to identify the problem, it is better to avoid having an useless line with :
        // ERROR: <blank>
        // For this reason, applying a minimum threshold (arbitrary) of characters to allow it to be displayed as error
        if (output.size() > 5)
        {
            if (!getPluginLogger().isEnabledFor(log4cplus::DEBUG_LOG_LEVEL))
            {
                LOGERROR("The osquery process unexpected logs: " << output);
            }
            else
            {
                LOGDEBUG("Osquery logs: " << output);
            }
        }

        switch (exitcode)
        {
            case ECANCELED:
                throw Plugin::IOsqueryCrashed("crashed");
            case ENOEXEC:
                throw Plugin::IOsqueryCannotBeExecuted("Exec format error when executing " + osqueryPath);
            case EACCES:
                throw Plugin::IOsqueryCannotBeExecuted("Permission denied when executing " + osqueryPath);
            default:
                return;
        }
    }

    void OsqueryProcessImpl::requestStop()
    {
        std::lock_guard<std::mutex> lock { m_processMonitorSharedResource };
        if (m_processMonitorPtr)
        {
            m_processMonitorPtr->kill(SECONDS_UNTIL_OSQUERYD_SIGKILL);
        }
    }

    void OsqueryProcessImpl::startProcess(const std::string& processPath, const std::vector<std::string>& arguments)
    {
        std::lock_guard lock { m_processMonitorSharedResource };
        m_processMonitorPtr = Common::Process::createProcess();
        m_processMonitorPtr->setOutputLimit(OUTPUT_BUFFER_LIMIT_BYTES);
        OsqueryLogIngest ingester;
        m_processMonitorPtr->setOutputTrimmedCallback(ingester);
        m_processMonitorPtr->setFlushBufferOnNewLine(true);
        m_processMonitorPtr->exec(processPath, arguments, {});
    }

    void OsqueryProcessImpl::killAnyOtherOsquery()
    {
        Proc::ensureNoExecWithThisCommIsRunning(Plugin::osqueryBinaryName(), Plugin::osqueryPath());
    }
} // namespace Plugin