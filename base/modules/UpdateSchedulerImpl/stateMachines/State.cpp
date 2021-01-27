/* Copyright 2012-2020 Sophos Limited. All rights reserved.
 * Sophos and Sophos Anti-Virus are registered trademarks of Sophos Limited.
 * All other product and company names mentioned are trademarks or registered
 * trademarks of their respective owners.
 */

#include "State.h"

#include "StringConversions.h"
//#include "RegistryAccess.h"

//#include <experimental/filesystem>
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

//void State::AddComponentState(const ComponentState& componentState)
//{
//	if (componentState.GetLineId().length() > 0)
//	{
//		m_componentStateMap[componentState.GetLineId()] = componentState;
//	}
//	else
//	{
//		m_componentStateMap[componentState.GetName()] = componentState;
//	}
//}

//void State::RemoveComponent(const std::string& lineId)
//{
//    auto iter = m_componentStateMap.find(lineId);
//    if (iter != m_componentStateMap.end())
//    {
//        m_componentStateMap.erase(iter);
//    }
//}

//ComponentState State::GetComponentState(const std::string& lineId, const std::string& name)
//{
//	if (m_componentStateMap.find(lineId) == m_componentStateMap.end())
//	{
//		bool replaceComponent = false;
//		ComponentState temp;
//
//		for (auto &csm : m_componentStateMap)
//		{
//			if (csm.first.compare(name) == 0)
//			{
//				temp = csm.second;
//				temp.SetLineId(lineId);
//				replaceComponent = true;
//			}
//		}
//
//		if (replaceComponent)
//		{
//			m_componentStateMap.erase(temp.GetName());
//			AddComponentState(temp);
//		}
//		else
//		{
//			AddComponentState(ComponentState(lineId));
//		}
//	}
//
//	return m_componentStateMap[lineId];
//}
	
//ComponentStateMap State::GetComponentStateMap() const
//{
//	return m_componentStateMap;
//}

//void State::ClearComponentStateMap()
//{
//	m_componentStateMap.clear();
//}

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

// Reboot handling logic:
// If called with anything other than RebootNotRequired then remember this.
// Disallow setting the state to a lower value.
//void State::SetRebootRequired(RebootData::RequiredEnum rebootRequired)
//{
//    if (rebootRequired > RebootData::RequiredEnum::RebootNotRequired)
//    {
//        rebootSetThisUpdate_ = true;
//    }
//
//    rebootStateMachine_.SignalRebootRequired(rebootRequired, std::chrono::system_clock::now());
//}
//
//void State::SignalScheduleTick()
//{
//    rebootStateMachine_.SignalScheduleTick();
//}

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

//RebootData::RequiredEnum State::GetRebootRequired() const
//{
//	return rebootStateMachine_.CurrentState().requiredEnum;
//}

//RebootData::MachineState State::GetRebootState() const
//{
//    return rebootStateMachine_.CurrentState();
//}

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

//void State::RawSetRebootRequired(const RebootData::MachineState& machineState)
//{
//    rebootStateMachine_ = StateLib::RebootStateMachine{ machineState };
//}

void State::RawSetDownloadState(const StateData::DownloadMachineState& machineState)
{
    downloadStateMachine_ = StateLib::DownloadStateMachine{ machineState, std::chrono::system_clock::now() };
}

void State::RawSetInstallState(const StateData::InstallMachineState& machineState)
{
    installStateMachine_ = StateLib::InstallStateMachine{ machineState, std::chrono::system_clock::now() };
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

bool State::SetMarketingVersionFromPath(const std::string& rootPath)
{
	// Sorry, but this needed to be in state so that it could be properly stubbed.
    
//	auto versionFilePath = std::experimental::filesystem::path(rootPath) / "version";
	//TO BE REMOVED AFTER
    if(rootPath.empty())
        return true;

//	std::ifstream ifs(versionFilePath);

//	if(!ifs.good())
//	{
//		SetMarketingVersion("");
//		return false;
//	}
//
//	std::streamoff begin = ifs.tellg();
//	ifs.seekg(0, std::ios::end);
//	std::streamoff end = ifs.tellg();
//	int size = (int)(end - begin);
//	ifs.seekg(0, std::ios::beg);
//
//	if(size > 256)
//	{
//		SetMarketingVersion("");
//		return false;
//	}
//
//	std::vector<char> vec;
//	vec.resize(size);
//
//	ifs.read(&vec[0], vec.size());
//
//	std::string version(vec.begin(), vec.end());
//
//	SetMarketingVersion(version);
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
	// Read the value that may or may not have been set in EBS,
	// then set the member variable.
	//
	// If the key is missing, set the empty string.

//	try
//	{
//		RegistryReader reader(HKEY_LOCAL_MACHINE, "Software\\Sophos\\AutoUpdate");
//		std::string type = reader.ReadStringValue("ProductType");
//
//		SetProductType(utf8_encode(type));
//	}
//	catch (const std::runtime_error&)
//	{
//		SetProductType("");
//	}
}
