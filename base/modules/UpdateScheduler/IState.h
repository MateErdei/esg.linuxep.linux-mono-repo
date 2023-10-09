// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#include "IVersionPersister.h"

#include "UpdateSchedulerImpl/stateMachinesModule/StateData.h"

#include <list>
#include <map>
#include <string>

struct SEUError
{
    SEUError(int exitCode, int stringID, const std::string& details) :
        stringID_(stringID), exitCode_(exitCode), details_(details)
    {
    }

    SEUError() {}

    int stringID_ = 0;
    int exitCode_ = 0;
    std::string details_ = "";
    std::list<std::string> inserts_;
};

struct IState
{
    virtual ~IState(){};

    virtual int ExitCode() = 0;

    virtual void SetLastSubscriptionVersions(LineVersionMap_t const& versions) = 0;
    virtual LineVersionMap_t GetLastSubscriptionVersions() const = 0;

    virtual void SetError(const SEUError& error) = 0;

    virtual SEUError GetLastError() const = 0;

    virtual void SignalDownloadResult(StateData::DownloadResultEnum downloadResult) = 0;
    virtual void SignalInstallResult(StateData::StatusEnum resultStatus) = 0;
    virtual bool DiscardEvent(int updateError) = 0;
    virtual StateData::DownloadMachineState GetDownloadState() const = 0;
    virtual StateData::InstallMachineState GetInstallState() const = 0;
    virtual StateData::EventMachineState GetEventState() const = 0;
    virtual bool GetRebootRequiredSetThisUpdate() = 0;
    virtual void SetForceRebootAfterThisUpdate(bool forceReboot) = 0;
    virtual bool GetForceRebootAfterThisUpdate() = 0;
    virtual void SetSuiteVersion(const std::string& version) = 0;
    virtual std::string GetSuiteVersion() = 0;
    virtual void SetMarketingVersion(const std::string& version) = 0;
    virtual std::string GetMarketingVersion() = 0;
    virtual bool SetMarketingVersionFromPath(const std::string& rootPath) = 0;
    virtual void SetProductType(const std::string& type) = 0;
    virtual std::string GetProductType() const = 0;

    virtual bool IsDelayedProductUpdate() const = 0;
    virtual void SetDelayedProductUpdate(bool isDelayedProductUpdate) = 0;

    virtual bool IsFallbackUpdate() const = 0;
    virtual void SetFallbackUpdate(bool isFallbackUpdate) = 0;

    // Internal method for the persister
    virtual void RawSetDownloadState(const StateData::DownloadMachineState& machineState) = 0;
    virtual void RawSetInstallState(const StateData::InstallMachineState& machineState) = 0;
    virtual void RawSetEventState(const StateData::EventMachineState& machineState) = 0;
};
