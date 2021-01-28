/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#pragma once

#include "DownloadStateMachine.h"
#include "EventStateMachine.h"
#include "InstallStateMachine.h"

#include "UpdateScheduler/IState.h"

#include <string>

class State: public IState
{
public:
	State();

	int ExitCode() override;

	void SetLastSubscriptionVersions(LineVersionMap_t const &versions) override;
	LineVersionMap_t GetLastSubscriptionVersions() const override;
	void SetError(const SEUError& error) override;
	SEUError GetLastError() const override;

    void SignalDownloadResult(StateData::DownloadResultEnum downloadResult) override;
    void SignalInstallResult(StateData::StatusEnum resultStatus) override;
    bool DiscardEvent(int updateError) override;
    StateData::DownloadMachineState GetDownloadState() const override;
    StateData::InstallMachineState GetInstallState() const override;
    StateData::EventMachineState GetEventState() const override;
	bool GetRebootRequiredSetThisUpdate() override;
	void SetForceRebootAfterThisUpdate(bool forceReboot) override;
	bool GetForceRebootAfterThisUpdate() override;
	void SetSuiteVersion(const std::string& version) override;
	std::string GetSuiteVersion() override;
	void SetMarketingVersion(const std::string& version) override;
	std::string GetMarketingVersion() override;
	bool SetMarketingVersionFromPath(const std::string& rootPath) override;
	void SetProductType(const std::string& type) override;
	std::string GetProductType() const override;
    bool IsDelayedProductUpdate() const override;
    void SetDelayedProductUpdate(bool isDelayedProductUpdate) override;
    bool IsFallbackUpdate() const override;
    void SetFallbackUpdate(bool isFallbackUpdate) override;

    void RawSetDownloadState(const StateData::DownloadMachineState& machineState) override;
    void RawSetInstallState(const StateData::InstallMachineState& machineState) override;
    void RawSetEventState(const StateData::EventMachineState& machineState) override;

private:
	void SetProductTypeFromRegistry();

	LineVersionMap_t m_lineVersions;
	SEUError lastError_;
    stateMachinesModule::DownloadStateMachine downloadStateMachine_;
    stateMachinesModule::InstallStateMachine installStateMachine_;
    stateMachinesModule::EventStateMachine eventStateMachine_;
	bool forceReboot_;
	bool rebootSetThisUpdate_;
    bool isDelayedProductUpdate_;
    bool isFallbackUpdate_;
	std::string suiteVersion_;
	std::string marketingVersion_;
	std::string productType_;
};
