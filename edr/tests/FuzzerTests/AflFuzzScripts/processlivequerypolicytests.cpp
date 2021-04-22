/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <modules/pluginimpl/PluginAdapter.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/Logging/LoggerConfig.h>


#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <unistd.h>

struct DevNullRedirect
{
    DevNullRedirect() : file("/dev/null")
    {
        strm_buffer = std::cout.rdbuf();
        strm_err_buffer = std::cerr.rdbuf();
        std::cout.rdbuf(file.rdbuf());
        std::cerr.rdbuf(file.rdbuf());
    }
    ~DevNullRedirect()
    {
        std::cout.rdbuf(strm_buffer);
        std::cerr.rdbuf(strm_err_buffer);
    }

    std::ofstream file;
    std::streambuf* strm_buffer;
    std::streambuf* strm_err_buffer;
};

class DummyServiceApli : public Common::PluginApi::IBaseServiceApi
{
public:
    void sendEvent(const std::string&, const std::string&) const override {};

    void sendStatus(const std::string&, const std::string&, const std::string&) const override {};

    void requestPolicies(const std::string&) const override {};
};

class TestablePluginAdapter : public Plugin::PluginAdapter
{
public:
    TestablePluginAdapter(std::shared_ptr<Plugin::QueueTask> queueTask) :
            Plugin::PluginAdapter(
                    queueTask,
                    std::unique_ptr<Common::PluginApi::IBaseServiceApi>(new DummyServiceApli()),
                    std::make_shared<Plugin::PluginCallback>(queueTask))
    {

    }
    void processALCPolicy(const std::string& policy, bool firstTime)
    {
        Plugin::PluginAdapter::processALCPolicy(policy, firstTime);
    }

    Plugin::OsqueryConfigurator& osqueryConfigurator()
    {
        return Plugin::PluginAdapter::osqueryConfigurator();
    }

    void ensureMCSCanReadOldResponses()
    {
        Plugin::PluginAdapter::ensureMCSCanReadOldResponses();
    }

    void setLiveQueryRevID(std::string revId)
    {
        m_liveQueryRevId = revId;
    }
    void setLiveQueryStatus(std::string status)
    {
        m_liveQueryStatus = status;
    }
    std::string getLiveQueryStatus()
    {
        return m_liveQueryStatus;
    }
    std::string getLiveQueryRevID()
    {
        return m_liveQueryRevId;
    }
    unsigned int getLiveQueryDataLimit()
    {
        return m_dataLimit;
    }

    void processLiveQueryPolicy(const std::string& policy)
    {
        Plugin::PluginAdapter::processLiveQueryPolicy(policy, true);
    }

    void setScheduleEpoch(time_t scheduleEpoch)
    {
        m_scheduleEpoch.setValue(scheduleEpoch);
    }
    time_t getScheduleEpochDuration()
    {
        return SCHEDULE_EPOCH_DURATION;
    }
};

int main()
{
    Common::Logging::ConsoleLoggingSetup consoleLoggingSetup{Common::Logging::LOGOFFFORTEST()};
    std::array<uint8_t, 9000> buffer;
    ssize_t read_bytes = ::read(STDIN_FILENO, buffer.data(), buffer.size());
    // failure to read is not to be considered in the parser. Just skip.
    if (read_bytes == -1)
    {
        perror("Failed to read");
        return 1;
    }
    DevNullRedirect devNullRedirect;
    std::string content(buffer.data(), buffer.data() + read_bytes);

    try
    {
        auto queueTask = std::make_shared<Plugin::QueueTask>();
        TestablePluginAdapter pluginAdapter(queueTask);
        pluginAdapter.processLiveQueryPolicy(content);
    }
    catch (std::exception &e)
    {
        return 2;
    }
    return 0;
}

