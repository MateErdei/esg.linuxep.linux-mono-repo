/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "State.h"

#include <fstream>

State::State() :
    //rebootStateMachine_{ RebootData::MachineState{} },
    downloadStateMachine_{ StateData::DownloadMachineState{}, std::chrono::system_clock::now() },
    installStateMachine_{ StateData::InstallMachineState{}, std::chrono::system_clock::now() },
    eventStateMachine_{ downloadStateMachine_, installStateMachine_, StateData::EventMachineState{} },
    forceReboot_(false), rebootSetThisUpdate_(false), isDelayedProductUpdate_(false), isFallbackUpdate_(false)
{
	// Set the product type as early as possible.
	// This does not depend on anything else, and is not expected
	// to change during execution.
	// Further, changing it any later (say during Sync, just before it's read)
	// could have unexpected side effects in unit tests, depending on what
	// the stubbed 'SetProductTypeFromRegistry' actually does.
	SetProductTypeFromRegistry();
}

int State::ExitCode()
{
	return lastError_.exitCode_;
}

void State::SetLastSubscriptionVersions(LineVersionMap_t const &versions)
{
	m_lineVersions = versions;
}

LineVersionMap_t State::GetLastSubscriptionVersions() const
{
	return m_lineVersions;
}

void State::SetError(const SEUError& error)
{
	lastError_ = error;
}

SEUError State::GetLastError() const
{
	return lastError_;
}

void State::SignalDownloadResult(StateData::DownloadResultEnum downloadResult)
{
    downloadStateMachine_.SignalDownloadResult(downloadResult, std::chrono::system_clock::now());
}

void State::SignalInstallResult(StateData::StatusEnum resultStatus)
{
    installStateMachine_.SignalInstallResult(resultStatus, std::chrono::system_clock::now());
}

bool State::DiscardEvent(int updateError)
{
    return eventStateMachine_.Discard(updateError, std::chrono::system_clock::now());
}

StateData::DownloadMachineState State::GetDownloadState() const
{
    return downloadStateMachine_.CurrentState();
}
StateData::InstallMachineState State::GetInstallState() const
{
    return installStateMachine_.CurrentState();
}

StateData::EventMachineState State::GetEventState() const
{
    return eventStateMachine_.CurrentState();
}

bool State::GetRebootRequiredSetThisUpdate()
{
	return rebootSetThisUpdate_;
}

void State::SetForceRebootAfterThisUpdate(bool forceReboot)
{
	forceReboot_ = forceReboot;
}

bool State::GetForceRebootAfterThisUpdate()
{
	return forceReboot_;
}

void State::RawSetDownloadState(const StateData::DownloadMachineState& machineState)
{
    downloadStateMachine_ = stateMachinesModule::DownloadStateMachine{ machineState, std::chrono::system_clock::now() };
}

void State::RawSetInstallState(const StateData::InstallMachineState& machineState)
{
    installStateMachine_ = stateMachinesModule::InstallStateMachine{ machineState, std::chrono::system_clock::now() };
}

void State::RawSetEventState(const StateData::EventMachineState& machineState)
{
    eventStateMachine_.Reset(machineState);
}

void State::SetSuiteVersion(const std::string& version)
{
	suiteVersion_ = version;
}

std::string State::GetSuiteVersion()
{
	return suiteVersion_;
}

void State::SetMarketingVersion(const std::string& version)
{
	marketingVersion_ = version;
}

std::string State::GetMarketingVersion()
{
	return marketingVersion_;
}

bool State::SetMarketingVersionFromPath(const std::string& /*rootPath*/)
{
	// Sorry, but this needed to be in state so that it could be properly stubbed.

	return true;
}

std::string State::GetProductType() const
{
	return productType_;
}

void State::SetProductType(const std::string& productType)
{
	productType_ = productType;
}

bool State::IsDelayedProductUpdate() const
{
    return isDelayedProductUpdate_;
}

void State::SetDelayedProductUpdate(bool isDelayedProductUpdate)
{
    isDelayedProductUpdate_ = isDelayedProductUpdate;
}

bool State::IsFallbackUpdate() const
{
    return isFallbackUpdate_;
}

void State::SetFallbackUpdate(bool isFallbackUpdate)
{
    isFallbackUpdate_ = isFallbackUpdate;
}

void State::SetProductTypeFromRegistry()
{
    // TODO LINUX-DAR 2590  May need to add implementation or remove.
}
