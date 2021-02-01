/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "StateData.h"
#include "StateMachineData.h"

#include <UpdateSchedulerImpl/configModule/UpdateEvent.h>
namespace UpdateSchedulerImpl::stateMachinesModule
{
    class StateMachineProcessor
    {
    public:
        StateMachineProcessor();

        void updateStateMachines(int updateResult);
        StateData::StateMachineData getStateMachineData() const;

    private:
        void populateStateMachines();
        void updateStateMachineData();
        void loadStateMachineRawData();
        void writeStataMachineRawData();

        StateData::StateMachineData m_stateMachineData;
        ::StateData::DownloadMachineState m_downloadMachineState{};
        ::StateData::InstallMachineState m_installMachineState{};
        ::StateData::EventMachineState m_eventMachineState{};
    };

}
