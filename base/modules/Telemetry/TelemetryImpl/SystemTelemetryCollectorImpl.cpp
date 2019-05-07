/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SystemTelemetryCollectorImpl.h"

#include <Telemetry/LoggerImpl/Logger.h>

#include <Common/Process/IProcessException.h>

#include <map>
#include <string>
#include <variant>

namespace Telemetry
{
    const int GL_kbSize = 1024 * 1024;
    const int GL_waitTimeMilliSeconds = 50;
    const int GL_waitMaxRetries = 10;

    SystemTelemetryCollectorImpl::SystemTelemetryCollectorImpl(
        Telemetry::SystemTelemetryConfig objectsConfig,
        Telemetry::SystemTelemetryConfig arraysConfig) :
        m_objectsConfig(std::move(objectsConfig)),
        m_arraysConfig(std::move(arraysConfig))
    {
    }

    template<typename T>
    std::map<std::string, T> SystemTelemetryCollectorImpl::collect(const SystemTelemetryConfig& config) const
    {
        m_commandOutputCache.clear();
        std::map<std::string, T> telemetry;
        for (auto const& [name, item] : config)
        {
            auto const& [command, commandArgs, regexp, properties] = item;
            std::string commandOutput;

            try
            {
                commandOutput = getTelemetryItem(command, commandArgs);
            }
            catch (const Common::Process::IProcessException& processException)
            {
                LOGERROR(
                    "Failed to get telemetry item: " << name << ", from command: " << command
                                                     << ", exception: " << processException.what());
                continue;
            }

            T values;

            if (getValues(values, commandOutput, regexp, properties)) // getValues used depends on T
            {
                telemetry[name] = values;
                continue;
            }

            LOGDEBUG("Failed to find telemetry item: " << name << ", from output of command: " << command);
        }

        return telemetry;
    }

    std::map<std::string, TelemetryItem> SystemTelemetryCollectorImpl::collectObjects() const
    {
        return collect<TelemetryItem>(m_objectsConfig);
    }

    std::map<std::string, std::vector<TelemetryItem>> SystemTelemetryCollectorImpl::collectArraysOfObjects() const
    {
        return collect<std::vector<TelemetryItem>>(m_arraysConfig);
    }

    std::string SystemTelemetryCollectorImpl::getTelemetryItem(const std::string& command, const std::string& args)
        const
    {
        if (m_commandOutputCache.find((command + args)) != m_commandOutputCache.end())
        {
            return m_commandOutputCache[(command + args)];
        }

        std::istringstream argsStream(args);
        std::vector<std::string> commandArgs(
            std::istream_iterator<std::string>{ argsStream }, std::istream_iterator<std::string>());

        auto processPtr = Common::Process::createProcess();
        processPtr->setOutputLimit(GL_kbSize);

        // gather raw telemetry, ignoring failures
        processPtr->exec(command, commandArgs);
        if (processPtr->wait(Common::Process::milli(GL_waitTimeMilliSeconds), GL_waitMaxRetries) !=
            Common::Process::ProcessStatus::FINISHED)
        {
            processPtr->kill();
            throw Common::Process::IProcessException(
                "Process execution timed out running: '" + command + " " + args + "'");
        }

        auto output = processPtr->output();

        int exitCode = processPtr->exitCode();
        if (exitCode != 0)
        {
            std::string reason = strerror(exitCode);
            throw Common::Process::IProcessException(
                "Process execution returned non-zero exit code, 'Exit Code: " + reason + "'");
        }

        m_commandOutputCache[(command + args)] = output;
        return output;
    }

    std::vector<std::string> SystemTelemetryCollectorImpl::matchSingleLine(
        std::istringstream& stream,
        const std::regex& re) const
    {
        std::string line;
        while (std::getline(stream, line))
        {
            std::smatch telemetryMatch;
            if (std::regex_search(line, telemetryMatch, re))
            {
                // return matching groups only
                return std::vector<std::string>(telemetryMatch.cbegin() + 1, telemetryMatch.cend());
            }
        }
        return {};
    }

    bool SystemTelemetryCollectorImpl::getValues(
        TelemetryItem& values,
        std::string& commandOutput,
        const std::string& regexp,
        std::vector<TelemetryProperty> properties) const
    {
        std::istringstream stream(commandOutput);
        std::regex re(regexp);

        auto singleLineMatches = matchSingleLine(stream, re);

        if (singleLineMatches.size() != properties.size())
        {
            return false;
        }

        for (size_t matchGroup = 0; matchGroup < singleLineMatches.size(); ++matchGroup)
        {
            std::pair<std::string, std::variant<std::string, int>> value;
            value.first = properties[matchGroup].name;

            if (properties[matchGroup].type == TelemetryValueType::INTEGER)
            {
                try
                {
                    value.second.emplace<1>(std::stoi(singleLineMatches[matchGroup]));
                    values.emplace_back(value);
                }
                catch (const std::invalid_argument& e)
                {
                    LOGDEBUG(
                        "Failed to find integer from output: " << singleLineMatches[matchGroup]
                                                               << ". Error message: " << e.what());
                    break;
                }
                catch (const std::out_of_range& e)
                {
                    LOGDEBUG(
                        "Failed to find integer from output: " << singleLineMatches[matchGroup]
                                                               << ". Error message: " << e.what());
                    break;
                }
            }
            else // default TelemetryValueType::STRING
            {
                value.second.emplace<0>(singleLineMatches[matchGroup]);
                values.emplace_back(value);
            }
        }

        return values.size() == properties.size();
    }

    bool SystemTelemetryCollectorImpl::getValues(
        std::vector<TelemetryItem>& values,
        std::string& commandOutput,
        const std::string& regexp,
        std::vector<TelemetryProperty> properties) const
    {
        std::istringstream stream(commandOutput);
        std::string lineFromCommandOutput;
        while (std::getline(stream, lineFromCommandOutput))
        {
            TelemetryItem valuesFromLine;

            if (getValues(valuesFromLine, lineFromCommandOutput, regexp, properties))
            {
                values.emplace_back(valuesFromLine);
            }
        }

        return !values.empty();
    }
} // namespace Telemetry
