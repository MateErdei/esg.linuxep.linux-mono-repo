/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.


 WatchdogApiTest enables to Fuzz the two Watchdog Inputs Socket as defined by the PluginApiMessage.proto and the wdctl
protocol By adding files like:

messages {
  wdservice {
    apimessage {
      pluginName: "mdr"
      command: DoAction
      applicationId: "MDR"
      payload: "TriggerUpdate"
    }
  }
}
messages {
  wdctl {
    commands: "ISRUNNING"
    commands: "mdr"
  }
}
******************************************************************************************************/

#include "FuzzerUtils.h"

#include "google/protobuf/text_format.h"

#include <Common/DirectoryWatcherImpl/Logger.h>
#include <Common/FileSystemImpl/FilePermissionsImpl.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/Logging/LoggerConfig.h>
#include <Common/ProcessImpl/ProcessImpl.h>
#include <Common/TelemetryHelperImpl/TelemetryJsonToMap.h>
#include <modules/Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <modules/Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <modules/Common/FileSystem/IFileSystem.h>
#include <modules/Common/PluginApi/ApiException.h>
#include <modules/Common/UtilityImpl/StringUtils.h>
#include <modules/Common/ZMQWrapperApi/IContext.h>
#include <modules/Common/ZMQWrapperApi/IContextSharedPtr.h>
#include <modules/Common/ZeroMQWrapper/IIPCException.h>
#include <modules/Common/ZeroMQWrapper/ISocketRequester.h>
#include <modules/ManagementAgent/ManagementAgentImpl/ManagementAgentMain.h>
#include <tests/Common/Helpers/FilePermissionsReplaceAndRestore.h>
#include <tests/Common/Helpers/TempDir.h>
#include <watchdog/watchdogimpl/Watchdog.h>

#include <future>
#include <stddef.h>
#include <stdint.h>
#include <thread>
#include <watchdogmessage.pb.h>

#ifdef HasLibFuzzer
#    include <libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h>
#    include <libprotobuf-mutator/src/mutator.h>
#endif

namespace
{
#ifdef HasLibFuzzer
#    define TESTLOG(x)
#else
#    define TESTLOG(x) std::cout << x
#endif

} // namespace

/** this class is just to allow the tests to be executed without requiring root*/
class NullFilePermission : public Common::FileSystem::FilePermissionsImpl
{
public:
    void chown(const Path&, const std::string&, const std::string&) const override {}
    void chmod(const Path&, __mode_t) const override {}
};

class ScopedFilePermission
{
public:
    ScopedFilePermission()
    {
        Tests::replaceFilePermissions(std::unique_ptr<Common::FileSystem::IFilePermissions>(new NullFilePermission()));
    }
    ~ScopedFilePermission() { Tests::restoreFilePermissions(); }
};

/** FakeProcess is here to avoid requiring to really start/stop plugins. This FakeProcess simulates the execution of the
 * plugins. It has some special provision for the systemctl as it is used to trigger update and usually does not take
 * long time, it is not a service.
 */
class FakeProcess : public Common::Process::IProcess
{
    Common::Process::ProcessStatus m_status;
    std::string m_path;

public:
    FakeProcess() : m_status{ Common::Process::ProcessStatus::NOTSTARTED } {}
    ~FakeProcess() { m_status = Common::Process::ProcessStatus::FINISHED; }
    void exec(
        const std::string& path,
        const std::vector<std::string>& arguments,
        const Common::Process::EnvPairVector& extraEnvironment,
        uid_t uid,
        gid_t gid) override
    {
        exec(path, arguments);
    }
    void exec(
        const std::string& path,
        const std::vector<std::string>& arguments,
        const std::vector<Common::Process::EnvironmentPair>& extraEnvironment) override
    {
        exec(path, arguments);
    };
    void exec(const std::string& path, const std::vector<std::string>& arguments) override
    {
        m_status = Common::Process::ProcessStatus::RUNNING;
        m_path = path;
        if (m_path.find("systemctl") != std::string::npos)
        {
            m_status = Common::Process::ProcessStatus::FINISHED;
        }
    };
    Common::Process::ProcessStatus wait(Common::Process::Milliseconds period, int attempts) override
    {
        if (m_status == Common::Process::ProcessStatus::NOTSTARTED)
        {
            return Common::Process::ProcessStatus::NOTSTARTED;
        }
        for (int i = 0; i < attempts; i++)
        {
            std::this_thread::sleep_for(period);
            if (m_status == Common::Process::ProcessStatus::FINISHED)
            {
                return Common::Process::ProcessStatus::FINISHED;
            }
        }
        return Common::Process::ProcessStatus::TIMEOUT;
    };
    int childPid() const override { return 12; }
    bool kill() override
    {
        m_status = Common::Process::ProcessStatus::FINISHED;
        return true;
    }
    bool kill(int secondsBeforeSIGKILL) override { return kill(); }
    int exitCode() override { return 0; };
    std::string output() override
    {
        while (wait(Common::Process::Milliseconds{ 5 }, 50) != Common::Process::ProcessStatus::FINISHED)
        {
        }
        return std::string();
    };
    Common::Process::ProcessStatus getStatus() override { return m_status; };
    void setOutputLimit(size_t limit) override{};
    void setOutputTrimmedCallback(std::function<void(std::string)>) override{};
    void setNotifyProcessFinishedCallBack(functor) override{};
    void waitUntilProcessEnds() override{};
};

class ScopedProcess
{
public:
    ScopedProcess()
    {
        Common::ProcessImpl::ProcessFactory::instance().replaceCreator([]() {
            std::unique_ptr<Common::Process::IProcess> proc{ new FakeProcess };
            return proc;
        });
    }
    ~ScopedProcess() { Common::ProcessImpl::ProcessFactory::instance().restoreCreator(); }
};

/** Helper function to create the json entry for the plugin*/
std::string templatePlugin(const std::string& installdir, const std::string& pluginname)
{
    std::string templ{ R"({
  "policyAppIds": [
    "MDR"
  ],
  "statusAppIds": [
    "MDR"
  ],
  "pluginName": "@plugin@",
  "executableFullPath": "@path@",
  "executableArguments": [ "9" ],
  "environmentVariables": [
    {
      "name": "SOPHOS_INSTALL",
      "value": "@@SOPHOS_INSTALL@@"
    }
  ],
  "executableUserAndGroup": "@@user@@:@@group@@"
})" };
    NullFilePermission nullFilePermission;

    std::string user = nullFilePermission.getUserName(::getuid());
    std::string group = nullFilePermission.getGroupName(::getgid());
    return Common::UtilityImpl::StringUtils::orderedStringReplace(
        templ,
        { { "@plugin@", pluginname },
          { "@path@", pluginname },
          { "@@SOPHOS_INSTALL@@", installdir },
          { "@@user@@", user },
          { "@@group@@", group } });
}

/**
 * Setup of files and directories as expected by Watchdog to be present.
 * As well as simple plugin registry for watchdog to use.
 */
void setupTestFiles(Tests::TempDir& tempdir)
{
    std::string installRoot = tempdir.dirPath();
    auto fileS = Common::FileSystem::fileSystem();

    for (std::string dir : { "base", "base/etc", "base/pluginRegistry", "var/ipc/plugins", "logs" })
    {
        std::string fullPath = Common::FileSystem::join(installRoot, dir);
        if (!fileS->isDirectory(fullPath))
        {
            fileS->makedirs(fullPath);
        }
    }
    tempdir.createFile("base/pluginRegistry/mdr.json", templatePlugin(installRoot, "mdr"));
    tempdir.createFile("base/pluginRegistry/mtr.json", templatePlugin(installRoot, "mtr"));
    tempdir.createFile("mdr", "");
    tempdir.createFile("mtr", "");

    std::string loggerPath = Common::FileSystem::join(installRoot, "base/etc/logger.conf");

    if (!fileS->isFile(loggerPath))
    {
        fileS->writeFile(loggerPath, "VERBOSITY=OFF");
    }
}

/**
 * This class holds the ManagementAgent instance running on its own thread and the zmq socket to communicate with
 * the management agent.
 * It is constructed as static object from the fuzzer test to allow the management agent to be running for the whole
 * test phase.
 *
 */

class WatchdogRunner : public Runner
{
public:
    WatchdogRunner() : Runner(), m_tempdir{ "/tmp" }
    {
        m_contextPtr = Common::ZMQWrapperApi::createContext();
        setupTestFiles(m_tempdir);
        Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, m_tempdir.dirPath());
        Common::ApplicationConfiguration::applicationConfiguration().setData(
            "plugins/watchdogservice.ipc", "inproc://service.ipc");
        Common::ApplicationConfiguration::applicationConfiguration().setData("watchdog.ipc", "inproc://watchdog.ipc");

        setMainLoop([this]() {
            ScopedFilePermission scopedFilePermission;
            ScopedProcess scopedProcess;
            try
            {
                watchdog::watchdogimpl::Watchdog m{ m_contextPtr };
                m.initialiseAndRun();
                TESTLOG("watchdog finished");
            }
            catch (std::exception& ex)
            {
                std::cerr << "Failed in the watchdog main" << std::endl;
                std::cerr << ex.what() << std::endl;
                // The goal is that Watchdog should not crash nor exit when receiving requests over the socket.
                // Calling abort is the way to allow LibFuzzer to pick-up the crash.
                abort();
            }
        });

        std::this_thread::sleep_for(std::chrono::seconds(2));

        m_requester = m_contextPtr->getRequester();
        std::string mng_address =
            Common::ApplicationConfiguration::applicationPathManager().getPluginSocketAddress("watchdogservice");
        m_requester->setTimeout(1000);
        m_requester->connect(mng_address);

        m_wdctlRequester = m_contextPtr->getRequester();
        mng_address = Common::ApplicationConfiguration::applicationPathManager().getWatchdogSocketAddress();
        m_wdctlRequester->setTimeout(1000);
        m_wdctlRequester->connect(mng_address);
    }

    void getWdctl(const std::vector<std::string>& command)
    {
        try
        {
            m_wdctlRequester->write(command);
            auto answ = m_wdctlRequester->read();
            if (answ.size() != 1)
            {
                std::cerr << "Wdctl replies are expected to be of size 1, it was " << answ.size() << std::endl;
                for (auto& e : answ)
                {
                    std::cerr << "Element : " << e << std::endl;
                }
                abort();
            }
        }
        catch (Common::ZeroMQWrapper::IIPCException& ex)
        {
            TESTLOG("wdctl error " << ex.what() << ". To be ignored");
        }
        catch (std::exception& ex)
        {
            // The 'contract' of the protocol and message is that the only acceptable
            // exception is the ApiException. Any other is not valid and that
            // is the reason abort is called.
            std::cerr << "Failed in getWdctl" << std::endl;
            std::cerr << ex.what() << std::endl;
            abort();
        }
    }

    /** Helper method to allow sending requests to Management Agent*/
    void getReply(const std::string& content)
    {
        try
        {
            TESTLOG("GetReply: " << content);
            m_requester->write({ content });
            auto reply = m_requester->read();
            if (!reply.empty())
            {
                TESTLOG("Answer: " << reply.at(0));
            }
            Common::PluginProtocol::Protocol protocol;
            auto message = protocol.deserialize(reply);
            if (message.m_command == Common::PluginProtocol::Commands::REQUEST_PLUGIN_TELEMETRY &&
                message.m_error.empty())
            {
                if (message.m_payload.empty())
                {
                    std::cerr << "Empty telemetry" << std::endl;
                    abort();
                }
                std::string telemetry = message.m_payload.at(0);

                try
                {
                    Common::Telemetry::flatJsonToMap(telemetry);
                }
                catch (std::exception& ex)
                {
                    std::cerr << "telemetry returned invalid json: " << ex.what() << std::endl;
                    abort();
                }
            }
            if (message.m_command == Common::PluginProtocol::Commands::REQUEST_PLUGIN_DO_ACTION &&
                message.m_error.empty())
            {
                if (!message.m_acknowledge)
                {
                    std::cerr << "It expects an acknowledge: " << std::endl;
                    abort();
                }
            }
        }
        catch (Common::PluginApi::ApiException& ex)
        {
            TESTLOG("Watchdog service api exception: " << ex.what() << ". To be ignored");
        }
        catch (Common::ZeroMQWrapper::IIPCException& ex)
        {
            TESTLOG("Watchdog service error: " << ex.what() << ". To be ignored");
        }
        catch (std::exception& ex)
        {
            // The 'contract' of the protocol and message is that the only acceptable
            // exception is the ApiException. Any other is not valid and that
            // is the reason abort is called.
            std::cout << "Failed in getReply" << std::endl;
            std::cout << ex.what() << std::endl;
            abort();
        }
    }

    bool managemementAgentRunning() { return threadRunning(); }

private:
    Tests::TempDir m_tempdir;
    Common::ZMQWrapperApi::IContextSharedPtr m_contextPtr;
    Common::ZeroMQWrapper::ISocketRequesterPtr m_requester;
    Common::ZeroMQWrapper::ISocketRequesterPtr m_wdctlRequester;
};

#ifdef HasLibFuzzer
DEFINE_PROTO_FUZZER(const PluginProtocolProto::WatchdogMsgs& messages)
{
#else
void mainTest(const PluginProtocolProto::WatchdogMsgs& messages)
{
#endif
    // It is static to ensure that only one watchdogRunner will exist while the full fuzz test runs.
    // this also means that the 'state' is not dependent only on the input message which may become a problem
    // but allows for a very robust verification of the watchdogRunner as it will be test througout the whole
    // fuzz test.
    static WatchdogRunner watchdogRunner;

    for (const PluginProtocolProto::WatchdogMsg& message : messages.messages())
    {
        if (message.has_wdctl())
        {
            std::vector<std::string> commands;
            for (auto& cmd : message.wdctl().commands())
            {
                commands.push_back(cmd);
            }
            watchdogRunner.getWdctl(commands);
        }
        if (message.has_wdservice())
        {
            std::string onTheWire = message.wdservice().apimessage().SerializeAsString();
            watchdogRunner.getReply(onTheWire);
        }

        if (!watchdogRunner.managemementAgentRunning())
        {
            std::cerr << "Management agent may have crashed";

            abort();
        }
    }
    TESTLOG("test finished");
}

/**
 * LibFuzzer works only with clang, and developers machine are configured to run gcc.
 * For this reason, the flag HasLibFuzzer has been used to enable buiding 2 outputs:
 *   * the real fuzzer tool
 *   * An output that is capable of consuming the same sort of input file that is used by the fuzzer
 *     but can be build and executed inside the developers IDE.
 */
#ifndef HasLibFuzzer
int main(int argc, char* argv[])
{
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
    PluginProtocolProto::WatchdogMsgs messages;

    if (argc < 2)
    {
        std::cerr << " Invalid argument. Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }
    std::string content = Common::FileSystem::fileSystem()->readFile(argv[1]);
    google::protobuf::TextFormat::Parser parser;
    parser.AllowPartialMessage(true);
    if (!parser.ParseFromString(content, &messages))
    {
        std::cerr << "Failed to parse file. Content: " << content << std::endl;
        return 3;
    }

    mainTest(messages);
    return 0;
}

#endif
