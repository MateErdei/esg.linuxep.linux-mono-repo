// Copyright 2021-2023 Sophos Limited. All rights reserved.
#pragma once

#include <ctime>
#include <string>
#include <vector>

namespace UpdateSchedulerImpl::StateData
{
    class StateMachineData
    {
    public:
        StateMachineData();

        static std::string toJsonStateMachineData(const StateMachineData& state);

        static StateMachineData fromJsonStateMachineData(const std::string& serializedVersion);

        const std::string& getDownloadFailedSinceTime() const;

        const std::string& getDownloadState() const;

        const std::string& getDownloadStateCredit() const;

        const std::string& getEventStateLastError() const;

        const std::string& getEventStateLastTime() const;

        const std::string& getInstallFailedSinceTime() const;

        const std::string& getInstallState() const;

        const std::string& getInstallStateCredit() const;

        const std::string& getLastGoodInstallTime() const;

        void setDownloadFailedSinceTime(const std::string& downloadFailedSinceTime);

        void setDownloadState(const std::string& downloadState);

        void setDownloadStateCredit(const std::string& downloadStateCredit);

        void setEventStateLastError(const std::string& eventStateLastError);

        void setEventStateLastTime(const std::string& eventStateLastTime);

        void setInstallFailedSinceTime(const std::string& installFailedSinceTime);

        void setInstallState(const std::string& installState);

        void setInstallStateCredit(const std::string& installStateCredit);

        void setLastGoodInstallTime(const std::string& lastGoodInstallTime);

        void setCanSendEvent(bool canSendEvent);

        bool canSendEvent() const;

    private:
        std::string m_downloadFailedSinceTime;
        std::string m_downloadState;
        std::string m_downloadStateCredit;
        std::string m_eventStateLastError;
        std::string m_eventStateLastTime;
        std::string m_installFailedSinceTime;
        std::string m_installState;
        std::string m_installStateCredit;
        std::string m_lastGoodInstallTime;
        bool m_canSendEvent;
    };
} // namespace UpdateSchedulerImpl::StateData
