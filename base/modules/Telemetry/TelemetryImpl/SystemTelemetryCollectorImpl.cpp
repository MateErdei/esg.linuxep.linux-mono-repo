/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "SystemTelemetryCollectorImpl.h"

#include <Common/Process/IProcessException.h>
#include <Common/UtilityImpl/StrError.h>
#include <Telemetry/LoggerImpl/Logger.h>

#include <algorithm>
#include <map>
#include <string>
#include <variant>
#include <Common/FileSystem/IFileSystem.h>

namespace Telemetry
{
    const int GL_mbSize = 1024 * 1024;

    SystemTelemetryCollectorImpl::SystemTelemetryCollectorImpl(
        Telemetry::SystemTelemetryConfig objectsConfig,
        Telemetry::SystemTelemetryConfig arraysConfig,
        unsigned waitTimeMilliSeconds,
        unsigned waitMaxRetries) :
        m_objectsConfig(std::move(objectsConfig)),
        m_arraysConfig(std::move(arraysConfig)),
        m_waitTimeMilliSeconds(waitTimeMilliSeconds),
        m_waitMaxRetries(waitMaxRetries)
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
                LOGDEBUG("Got telemetry item: " << name << ", from command: " << command);
            }
            catch (const Common::Process::IProcessException& processException)
            {
                LOGERROR(
                    "Failed to get telemetry item: " << name << ", from command: " << command
                                                     << ", exception: " << processException.what());
                continue;
            }
            catch(const std::invalid_argument& invalidArg)
            {
                LOGWARN(
                        "Failed to get telemetry item: " << name << ", could not find executable for command: " << command
                                                         << ", exception: " << invalidArg.what());
            }

            T values;

            if (getTelemetryValuesFromCommandOutput(
                    values, commandOutput, regexp, properties)) // getTelemetryValuesFromCommandOutput used depends on T
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

    std::string SystemTelemetryCollectorImpl::getTelemetryItem(
        const std::string& command,
        std::vector<std::string> args) const
    {
        std::string commandAndArgs(command);
        std::for_each(
            args.begin(), args.end(), [&commandAndArgs](const std::string& arg) { commandAndArgs.append(arg); });

        if (m_commandOutputCache.find(commandAndArgs) != m_commandOutputCache.end())
        {
            return m_commandOutputCache[commandAndArgs];
        }

        auto processPtr = Common::Process::createProcess();
        processPtr->setOutputLimit(GL_mbSize);

        auto commandExecutablePath = getSystemCommandExecutablePath(command);

        // gather raw telemetry, ignoring failures
        processPtr->exec(commandExecutablePath, args);
        if (processPtr->wait(Common::Process::milli(m_waitTimeMilliSeconds), m_waitMaxRetries) !=
            Common::Process::ProcessStatus::FINISHED)
        {
            processPtr->kill();
            throw Common::Process::IProcessException("Process execution timed out running: '" + commandAndArgs + "'");
        }

        int exitCode = processPtr->exitCode();
        if (exitCode != 0)
        {
            if (exitCode == 2)
            {
                LOGWARN("Process execution was unsuccessful, command not found: '" + commandAndArgs + "'");
                return m_commandOutputCache[commandAndArgs];
            }
            else
            {
                throw Common::Process::IProcessException(
                        "Process execution returned non-zero exit code, 'Exit Code: [" +
                        std::to_string(exitCode) +
                        "] " +
                        Common::UtilityImpl::StrError(exitCode) + "'");
            }
        }

        auto output = processPtr->output();
        m_commandOutputCache[commandAndArgs] = output;
        return output;
    }

    std::vector<std::string> SystemTelemetryCollectorImpl::matchSingleLine(
        std::istringstream& stream,
        const std::regex& re) const
    {
        std::string line;
        while (std::getline(stream, line))
        {
            std::smatch telemetryMatches;
            if (std::regex_search(line, telemetryMatches, re) && telemetryMatches.size() > 1)
            {
                // skip first match (for whole line) and return subexpression groups only
                return std::vector<std::string>(telemetryMatches.cbegin() + 1, telemetryMatches.cend());
            }
        }

        return std::vector<std::string>();
    }

    bool SystemTelemetryCollectorImpl::getTelemetryValuesFromCommandOutput(
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
                    value.second = std::stoi(singleLineMatches[matchGroup]);
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
                value.second = singleLineMatches[matchGroup];
                values.emplace_back(value);
            }
        }

        return values.size() == properties.size();
    }

    bool SystemTelemetryCollectorImpl::getTelemetryValuesFromCommandOutput(
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

            if (getTelemetryValuesFromCommandOutput(valuesFromLine, lineFromCommandOutput, regexp, properties))
            {
                values.emplace_back(valuesFromLine);
            }
        }

        return !values.empty();
    }

    std::string SystemTelemetryCollectorImpl::getSystemCommandExecutablePath(const std::string& executableName) const
    {
        std::vector<std::string> folderLocations = { "/usr/bin", "/bin", "/usr/local/bin", "/sbin", "/usr/sbin" };
        for (const auto& folder : folderLocations)
        {
            Path path = Common::FileSystem::join(folder, executableName);
            if (Common::FileSystem::fileSystem()->isExecutable(path))
            {
                return path;
            }
        }
        throw std::invalid_argument("Executable " + executableName + " is not installed.");
    }
} // namespace Telemetry
