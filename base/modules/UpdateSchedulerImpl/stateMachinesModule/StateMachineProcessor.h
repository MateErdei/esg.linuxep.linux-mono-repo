// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#include "StateData.h"

#include "UpdateSchedulerImpl/common/StateMachineData.h"

namespace UpdateSchedulerImpl::stateMachinesModule
{
    class StateMachineProcessor
    {
    public:
        StateMachineProcessor(const std::string& lastInstallTime);

        void updateStateMachines(const int updateResult);
        StateData::StateMachineData getStateMachineData() const;

    private:
        void populateStateMachines();
        void updateStateMachineData();
        void loadStateMachineRawData();
        void writeStateMachineRawData();

        int m_currentDownloadStateResult;
        int m_currentInstallStateResult;
        std::string m_lastInstallTime;
        StateData::StateMachineData m_stateMachineData;
        ::StateData::DownloadMachineState m_downloadMachineState{};
        ::StateData::InstallMachineState m_installMachineState{};
        ::StateData::EventMachineState m_eventMachineState{};
    };

} // namespace UpdateSchedulerImpl::stateMachinesModule
