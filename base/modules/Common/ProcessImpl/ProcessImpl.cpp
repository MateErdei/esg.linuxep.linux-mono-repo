// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "ProcessImpl.h"

#include "BoostProcessHolder.h"
#include "Logger.h"

#include "Common/Process/IProcessException.h"

#include <algorithm>
#include <future>
#include <iostream>
#include <pwd.h>
#include <queue>
#include <thread>
#include <utility>
#include <unistd.h>

std::unique_ptr<Common::Process::IProcess> Common::Process::createProcess()
{
    return ProcessImpl::ProcessFactory::instance().createProcess();
}

void Common::Process::replaceCreator(std::function<std::unique_ptr<Common::Process::IProcess>(void)> creator)
{
    ProcessImpl::ProcessFactory::instance().replaceCreator(std::move(creator));
}

void Common::Process::restoreCreator()
{
    ProcessImpl::ProcessFactory::instance().restoreCreator();
}

Common::Process::Milliseconds Common::Process::milli(int n_ms)
{
    return std::chrono::milliseconds(n_ms);
}

namespace Common::ProcessImpl
{
    /* In order to keep previous behaviour of ProcessImpl that would throw for some case in ::exec and not throw in
     * other cases of failure. And in order to avoid to have to keep track of the state of the process in the
     * ProcessImpl, there are two 'dummy' IProcessImpl
     *
     * Failed to StartProcess keep track of when the exec failed.
     */
    class FailedToStartProcess : public IProcessHolder
    {
        int m_errorCode;

    public:
        explicit FailedToStartProcess(int errorCode) : m_errorCode(errorCode) {}
        int pid() override { return -1; }

        void wait() override { }

        Process::ProcessStatus wait(std::chrono::milliseconds) override
        {
            return Process::ProcessStatus::FINISHED;
        };

        int exitCode() override { return m_errorCode; }
        int nativeExitCode() override { return m_errorCode; }

        std::string output() override { return ""; }
        std::string stderroutput() override { return ""; }
        std::string stdoutput() override { return ""; }

        bool hasFinished() override { return true; }

        void sendTerminateSignal() override { }
        void sendAbortSignal() override { }
        void sendUsr1Signal() override { };

        void kill() override { }
    };

    /* Initial state, keep track that no exec was ever called */
    class NoExecCalled : public IProcessHolder
    {
    public:
        int pid() override { return -1; }

        void wait() override { }

        Process::ProcessStatus wait(std::chrono::milliseconds) override
        {
            return Process::ProcessStatus::NOTSTARTED;
        };

        int exitCode() override
        {
            throw Process::IProcessException("Exit Code can be called only after exec and process exit.");
        }

        int nativeExitCode() override
        {
            throw Process::IProcessException("Exit Code can be called only after exec and process exit.");
        }

        std::string output() override { throw Process::IProcessException("Output can be called only after exec."); }
        std::string stderroutput() override { throw Process::IProcessException("Output can be called only after exec."); }
        std::string stdoutput() override { throw Process::IProcessException("Output can be called only after exec."); }

        bool hasFinished() override { return true; }

        void sendTerminateSignal() override { }
        void sendAbortSignal() override { }
        void sendUsr1Signal() override { };

        void kill() override { }
    };

    ProcessImpl::ProcessImpl() :
        m_pid{ -1 },
        m_outputLimit(0),
        m_flushOnNewLine(false),
        m_callback{ []() {} },
        m_notifyTrimmed{ [](std::string) {} }
    {
        m_d = std::make_shared<NoExecCalled>();
    }

    ProcessImpl::~ProcessImpl() = default;

    Process::ProcessStatus ProcessImpl::wait(Process::Milliseconds period, int attempts)
    {
        Process::Milliseconds fullPeriod = period * attempts;
        if (fullPeriod < Process::Milliseconds{ 1 })
        {
            fullPeriod = Process::Milliseconds{ 1 };
        }
        auto processOnBoost = safeAccess();
        return processOnBoost->wait(fullPeriod);
    }

    void ProcessImpl::exec(const std::string& path, const std::vector<std::string>& arguments)
    {
        this->exec(path, arguments, std::vector<Process::EnvironmentPair>{}, ::getuid(), ::getgid());
    }

    void ProcessImpl::exec(
        const std::string& path,
        const std::vector<std::string>& arguments,
        const std::vector<Process::EnvironmentPair>& extraEnvironment)
    {
        this->exec(path, arguments, extraEnvironment, ::getuid(), ::getgid());
    }

    void ProcessImpl::exec(
        const std::string& path,
        const std::vector<std::string>& arguments,
        const std::vector<Process::EnvironmentPair>& extraEnvironment,
        uid_t uid,
        gid_t gid)
    {
        std::lock_guard<std::mutex> lock(m_protectImplOnBoost);
        m_pid = -1;
        try
        {
            m_d = std::make_shared<BoostProcessHolder>(
                path,
                arguments,
                extraEnvironment,
                uid,
                gid,
                m_callback,
                m_notifyTrimmed,
                m_outputLimit,
                m_flushOnNewLine);
            m_pid = m_d->pid();
        }
        catch (Common::Process::IProcessException& ex)
        {
            LOGWARN("Failure to setup process: " << ex.what());
            m_d = std::make_shared<FailedToStartProcess>(2);
            throw;
        }
        catch (std::system_error& system_error)
        {
            LOGWARN("Failure to start process: " << system_error.what());
            m_d = std::make_shared<FailedToStartProcess>(system_error.code().value());
            m_callback();
        }
        catch (std::exception& ex)
        {
            LOGWARN("Failure to start process: " << ex.what());
            m_d = std::make_shared<FailedToStartProcess>(2);
            m_callback();
        }
    }
    int ProcessImpl::exitCode()
    {
        auto processOnBoost = safeAccess();
        return processOnBoost->exitCode();
    }

    int ProcessImpl::nativeExitCode()
    {
        auto processOnBoost = safeAccess();
        return processOnBoost->nativeExitCode();
    }

    std::string ProcessImpl::output()
    {
        auto processOnBoost = safeAccess();
        return processOnBoost->output();
    }

    std::string ProcessImpl::errorOutput()
    {
        auto processOnBoost = safeAccess();
        return processOnBoost->stderroutput();
    }

    std::string ProcessImpl::standardOutput()
    {
        auto processOnBoost = safeAccess();
        return processOnBoost->stdoutput();
    }

    bool ProcessImpl::kill() { return kill(2); }

    bool ProcessImpl::kill(int secondsBeforeSIGKILL)
    {
        int numOfDecSeconds = secondsBeforeSIGKILL * 10;
        bool requiredKill = false;
        auto processOnBoost = safeAccess();
        if (!processOnBoost->hasFinished())
        {
            processOnBoost->sendTerminateSignal();
            if (wait(Process::milli(numOfDecSeconds), 100) == Process::ProcessStatus::TIMEOUT)
            {
                if (m_coreDumpEnabled)
                {
                    processOnBoost->sendAbortSignal();
                }
                else
                {
                    processOnBoost->kill();
                }

                requiredKill = true;
            }
        }
        return requiredKill;
    }

    void ProcessImpl::sendSIGUSR1()
    {
        auto processOnBoost = safeAccess();
        processOnBoost->sendUsr1Signal();
    }

    Process::ProcessStatus ProcessImpl::getStatus()
    {
        auto processOnBoost = safeAccess();
        if (!processOnBoost)
        {
            LOGSUPPORT("getStatus can be called only after exec");
            return Process::ProcessStatus::NOTSTARTED;
        }

        if (processOnBoost->hasFinished())
        {
            return Process::ProcessStatus::FINISHED;
        }
        return Process::ProcessStatus::RUNNING;
    }

    void ProcessImpl::setOutputLimit(size_t limit) { m_outputLimit = limit; }

    void ProcessImpl::setFlushBufferOnNewLine(bool flushOnNewLine) { m_flushOnNewLine = flushOnNewLine; }

    void ProcessImpl::setNotifyProcessFinishedCallBack(Process::IProcess::functor callback)
    {
        m_callback = std::move(callback);
    }

    int ProcessImpl::childPid() const { return m_pid; }

    ProcessFactory::ProcessFactory() { restoreCreator(); }

    ProcessFactory& ProcessFactory::instance()
    {
        static ProcessFactory singleton;
        return singleton;
    }

    std::unique_ptr<Process::IProcess> ProcessFactory::createProcess() { return m_creator(); }

    void ProcessFactory::replaceCreator(std::function<std::unique_ptr<Process::IProcess>(void)> creator)
    {
        m_creator = std::move(creator);
    }

    void ProcessFactory::restoreCreator()
    {
        m_creator = []() {
            return std::unique_ptr<Common::Process::IProcess>(new Common::ProcessImpl::ProcessImpl());
        };
    }

    void ProcessImpl::waitUntilProcessEnds()
    {
        auto processOnBoost = safeAccess();
        processOnBoost->wait();
    }

    void ProcessImpl::setCoreDumpMode(const bool mode)
    {
        m_coreDumpEnabled = mode;
    }

    void ProcessImpl::setOutputTrimmedCallback(std::function<void(std::string)> outputTrimmedCallback)
    {
        m_notifyTrimmed = outputTrimmedCallback;
    }

    std::shared_ptr<IProcessHolder> ProcessImpl::safeAccess()
    {
        std::lock_guard<std::mutex> lock(m_protectImplOnBoost);
        std::shared_ptr<IProcessHolder> copy = m_d;
        return copy;
    }

} // namespace Common::ProcessImpl
