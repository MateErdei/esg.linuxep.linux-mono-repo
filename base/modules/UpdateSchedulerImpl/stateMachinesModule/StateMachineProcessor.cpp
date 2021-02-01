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
    StateMachineProcessor::StateMachineProcessor()
    {
        loadStateMachineRawData();
    }

    void StateMachineProcessor::populateStateMachines()
    {
        m_downloadMachineState.credit = std::stoi(m_stateMachineData.getDownloadStateCredit());
        long downloadFailedSince = std::stol(m_stateMachineData.getDownloadFailedSinceTime());
        m_downloadMachineState.failedSince =
            std::chrono::system_clock::time_point{ std::chrono::milliseconds{ downloadFailedSince } };

        m_installMachineState.credit = std::stoi(m_stateMachineData.getInstallStateCredit());
        long installFailedSince = std::stol(m_stateMachineData.getInstallFailedSinceTime());
        m_installMachineState.failedSince =
            std::chrono::system_clock::time_point{ std::chrono::milliseconds{ installFailedSince } };
        long installLassGood = std::stol(m_stateMachineData.getLastGoodInstallTime());
        m_installMachineState.lastGood =
            std::chrono::system_clock::time_point{ std::chrono::milliseconds{ installLassGood } };

        long eventLastTime = std::stol(m_stateMachineData.getEventStateLastTime());
        m_eventMachineState.lastTime =
            std::chrono::system_clock::time_point{ std::chrono::milliseconds{ eventLastTime } };
        m_eventMachineState.lastError = std::stoi(m_stateMachineData.getEventStateLastError());
    }

    void  StateMachineProcessor::updateStateMachineData()
    {
        m_stateMachineData.setDownloadStateCredit(std::to_string(m_downloadMachineState.credit));
        m_stateMachineData.setDownloadFailedSinceTime(std::to_string(
            std::chrono::duration_cast<std::chrono::milliseconds>(m_downloadMachineState.failedSince.time_since_epoch())
                .count()));

        m_stateMachineData.setInstallStateCredit(std::to_string(m_installMachineState.credit));
        m_stateMachineData.setInstallFailedSinceTime(std::to_string(
            std::chrono::duration_cast<std::chrono::milliseconds>(m_installMachineState.failedSince.time_since_epoch())
                .count()));

        m_stateMachineData.setLastGoodInstallTime(std::to_string(
            std::chrono::duration_cast<std::chrono::milliseconds>(m_installMachineState.lastGood.time_since_epoch())
                .count()));

        m_stateMachineData.setEventStateLastTime(std::to_string(
            std::chrono::duration_cast<std::chrono::milliseconds>(m_eventMachineState.lastTime.time_since_epoch())
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

    void StateMachineProcessor::writeStataMachineRawData()
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

    void StateMachineProcessor::updateStateMachines(int updateResult)
    {
        auto currentTime = std::chrono::system_clock::now();

        ::stateMachinesModule::DownloadStateMachine downloadStateMachine(m_downloadMachineState, currentTime);
        ::stateMachinesModule::InstallStateMachine  installStateMachine(m_installMachineState, currentTime);
        ::stateMachinesModule::EventStateMachine eventStateMachine(downloadStateMachine, installStateMachine, m_eventMachineState);

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
            installStateMachine.SignalInstallResult(::StateData::StatusEnum::bad, currentTime);
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
            installStateMachine.SignalInstallResult(::StateData::StatusEnum::bad, currentTime);
        }
        else
        {
            downloadStateMachine.SignalDownloadResult(::StateData::DownloadResultEnum::bad, currentTime);
            installStateMachine.SignalInstallResult(::StateData::StatusEnum::bad, currentTime);
        }

        m_downloadMachineState = downloadStateMachine.CurrentState();
        m_installMachineState = installStateMachine.CurrentState();
        m_eventMachineState = eventStateMachine.CurrentState();

        writeStataMachineRawData();
    }

    StateData::StateMachineData StateMachineProcessor::getStateMachineData() const
    {
        return m_stateMachineData;
    }
}