/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "StateMachineProcessor.h"

#include "EventStateMachine.h"
#include "InstallStateMachine.h"
#include "Logger.h"
#include "StateData.h"
#include "StateMachineData.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <UpdateSchedulerImpl/configModule/UpdateEvent.h>

namespace UpdateSchedulerImpl::stateMachinesModule
{
    StateMachineProcessor::StateMachineProcessor(const std::string& lastInstallTime)
    : m_currentDownloadStateResult(0)
    , m_currentInstallStateResult(0)
    , m_lastInstallTime(lastInstallTime)
    {
        loadStateMachineRawData();
    }

    void StateMachineProcessor::populateStateMachines()
    {
        m_downloadMachineState.credit =  m_stateMachineData.getDownloadStateCredit().empty() ? 0 : std::stoi(m_stateMachineData.getDownloadStateCredit());
        long downloadFailedSince = m_stateMachineData.getDownloadFailedSinceTime().empty() ? 0 : std::stol(m_stateMachineData.getDownloadFailedSinceTime());
        m_downloadMachineState.failedSince =
            std::chrono::system_clock::time_point{ std::chrono::seconds{ downloadFailedSince } };

        m_installMachineState.credit = m_stateMachineData.getInstallStateCredit().empty() ? 0 : std::stoi(m_stateMachineData.getInstallStateCredit());
        long installFailedSince = m_stateMachineData.getInstallFailedSinceTime().empty() ? 0 : std::stol(m_stateMachineData.getInstallFailedSinceTime());
        m_installMachineState.failedSince =
            std::chrono::system_clock::time_point{ std::chrono::seconds{ installFailedSince } };
        long installLastGood = m_stateMachineData.getLastGoodInstallTime().empty() ? 0 : std::stol(m_stateMachineData.getLastGoodInstallTime());
        m_installMachineState.lastGood =
            std::chrono::system_clock::time_point{ std::chrono::seconds{ installLastGood } };

        long eventLastTime = m_stateMachineData.getEventStateLastTime().empty() ? 0 : std::stol(m_stateMachineData.getEventStateLastTime());
        m_eventMachineState.lastTime =
            std::chrono::system_clock::time_point{ std::chrono::seconds{ eventLastTime } };
        m_eventMachineState.lastError = m_stateMachineData.getEventStateLastError().empty() ? 0 : std::stoi(m_stateMachineData.getEventStateLastError());
    }

    void  StateMachineProcessor::updateStateMachineData()
    {
        // take a copy of the stateMachineData for comparing transition of state below.
        StateData::StateMachineData currentStateMachineData = m_stateMachineData;

        m_stateMachineData.setDownloadState(std::to_string(m_currentDownloadStateResult));
        m_stateMachineData.setInstallState(std::to_string(m_currentInstallStateResult));
        m_stateMachineData.setDownloadStateCredit(std::to_string(m_downloadMachineState.credit));

        if (currentStateMachineData.getDownloadState() == "0" &&  m_stateMachineData.getDownloadState() == "1")
        {
            // if transitioning from good to bad download state update failed time.
            m_stateMachineData.setDownloadFailedSinceTime(std::to_string(
                    std::chrono::duration_cast<std::chrono::seconds>(m_downloadMachineState.failedSince.time_since_epoch())
                            .count()));
        }
        else if (m_stateMachineData.getDownloadState() == "0")
        {
            m_stateMachineData.setDownloadFailedSinceTime("0");
        }
        else
        {
            m_stateMachineData.setDownloadFailedSinceTime(currentStateMachineData.getDownloadFailedSinceTime());
        }

        m_stateMachineData.setInstallStateCredit(std::to_string(m_installMachineState.credit));

        if (currentStateMachineData.getInstallState() == "0" &&  m_stateMachineData.getInstallState() == "1")
        {
            m_stateMachineData.setInstallFailedSinceTime(std::to_string(
                std::chrono::duration_cast<std::chrono::seconds>(m_installMachineState.failedSince.time_since_epoch())
                    .count()));
        }
        else if (m_stateMachineData.getInstallState() == "0")
        {
            m_stateMachineData.setInstallFailedSinceTime("0");
        }
        else
        {
            m_stateMachineData.setInstallFailedSinceTime(currentStateMachineData.getInstallFailedSinceTime());
        }

        m_stateMachineData.setLastGoodInstallTime(m_lastInstallTime);

        m_stateMachineData.setEventStateLastTime(std::to_string(
            std::chrono::duration_cast<std::chrono::seconds>(m_eventMachineState.lastTime.time_since_epoch())
                .count()));

        m_stateMachineData.setEventStateLastError(std::to_string(m_eventMachineState.lastError));

    }

    void StateMachineProcessor::loadStateMachineRawData()
    {
        std::string stateMachineDataPath =
            Common::ApplicationConfiguration::applicationPathManager().getStateMachineRawDataPath();
        auto fileSystem = Common::FileSystem::fileSystem();
        if (fileSystem->exists(stateMachineDataPath))
        {
            try
            {
                std::string stateMachineDataContent = fileSystem->readFile(stateMachineDataPath);
                m_stateMachineData = StateData::StateMachineData::fromJsonStateMachineData(stateMachineDataContent);
                populateStateMachines();
            }
            catch (Common::FileSystem::IFileSystemException& ex)
            {
                LOGWARN("Failed to read the state machine data file : " << stateMachineDataPath);
            }
        }
        else
        {
            LOGINFO(
                "No state machine data file found at : " << stateMachineDataPath
                                                         << " , state machines are starting with default values.");
        }
    }

    void StateMachineProcessor::writeStateMachineRawData()
    {
        auto fileSystem = Common::FileSystem::fileSystem();
        std::string stateMachineDataPath =
            Common::ApplicationConfiguration::applicationPathManager().getStateMachineRawDataPath();

        updateStateMachineData();

        if (fileSystem->exists(stateMachineDataPath))
        {
            fileSystem->removeFile(stateMachineDataPath);
        }
        try
        {
            fileSystem->writeFile(
                stateMachineDataPath, StateData::StateMachineData::toJsonStateMachineData(m_stateMachineData));
        }
        catch (Common::FileSystem::IFileSystemException& ex)
        {
            LOGWARN("Failed to write the state machine data file : " << stateMachineDataPath);
        }
    }

    void StateMachineProcessor::updateStateMachines(const int updateResult)
    {
        auto currentTime = std::chrono::system_clock::now();

        ::stateMachinesModule::DownloadStateMachine downloadStateMachine(m_downloadMachineState, currentTime);
        ::stateMachinesModule::InstallStateMachine  installStateMachine(m_installMachineState, currentTime);

        if (updateResult == configModule::EventMessageNumber::SUCCESS)
        {
            downloadStateMachine.SignalDownloadResult(::StateData::DownloadResultEnum::good, currentTime);
            installStateMachine.SignalInstallResult(::StateData::StatusEnum::good, currentTime);
        }
        else if( (updateResult == configModule::EventMessageNumber::DOWNLOADFAILED)
                 || (updateResult == configModule::EventMessageNumber::MULTIPLEPACKAGEMISSING)
                 || (updateResult == configModule::EventMessageNumber::SINGLEPACKAGEMISSING))
        {
            downloadStateMachine.SignalDownloadResult(::StateData::DownloadResultEnum::bad, currentTime);
        }
        else if( (updateResult == configModule::EventMessageNumber::INSTALLCAUGHTERROR)
                 || (updateResult == configModule::EventMessageNumber::INSTALLFAILED))
        {
            downloadStateMachine.SignalDownloadResult(::StateData::DownloadResultEnum::good, currentTime);
            installStateMachine.SignalInstallResult(::StateData::StatusEnum::bad, currentTime);
        }
        else if( updateResult == configModule::EventMessageNumber::CONNECTIONERROR)
        {
            downloadStateMachine.SignalDownloadResult(::StateData::DownloadResultEnum::noNetwork, currentTime);
        }
        else
        {
            downloadStateMachine.SignalDownloadResult(::StateData::DownloadResultEnum::bad, currentTime);
            installStateMachine.SignalInstallResult(::StateData::StatusEnum::bad, currentTime);
        }

        m_currentDownloadStateResult = downloadStateMachine.getOverallState();
        m_currentInstallStateResult = installStateMachine.getOverallState();

        ::stateMachinesModule::EventStateMachine eventStateMachine(downloadStateMachine, installStateMachine, m_eventMachineState);

        int val = configModule::EventMessageNumber::MULTIPLEPACKAGEMISSING;
        LOGINFO(val);
        m_stateMachineData.setCanSendEvent(!eventStateMachine.Discard(updateResult, currentTime));

        m_downloadMachineState = downloadStateMachine.CurrentState();
        m_installMachineState = installStateMachine.CurrentState();
        m_eventMachineState = eventStateMachine.CurrentState();

        writeStateMachineRawData();
    }

    StateData::StateMachineData StateMachineProcessor::getStateMachineData() const
    {
        return m_stateMachineData;
    }
}