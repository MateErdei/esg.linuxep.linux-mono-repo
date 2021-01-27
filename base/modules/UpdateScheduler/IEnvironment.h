//------------------------------------------------------------------------------
// Copyright 2016-2020 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group. All
// other product and company names mentioned are trademarks or registered
// trademarks of their respective owners.
//------------------------------------------------------------------------------

#pragma once

#include "ICacheEvaluator.h"
#include "IHealthEventManager.h"
//#include "UpdateSchedulerImpl/stateMachines/RegistryAccess.h"


//#include "WindowsOnWindows.h"
#include "UpdateSchedulerImpl/stateMachines/Logger.h"

//#include <experimental/filesystem>
#include <string>
#include <string_view>
#include <vector>
#include <set>
#include <map>

namespace OSInfo
{
    struct Data
    {
        std::string platformToken;
        std::string platformRelease;
        unsigned int major;
        unsigned int minor;
        unsigned int build;
        bool isServer;
        bool is64BitOS;
        bool isARM64;
    };
}

namespace Flags {
    constexpr std::string_view UseSdds3 = "engineering.sdds3.available";
    constexpr std::string_view UseRepairKit = "repair.available";
}


// Stores cloud subscription data

struct CloudSubscriptionData
{
    std::string rigidName;
    std::string baseVersion;
    std::string releaseTag;
    std::string fixedVersion;
    void *handle;
};

typedef std::vector<CloudSubscriptionData> CloudSubscriptions;

// Stores Health event data

typedef std::map<std::string, std::string> HealthEvents;

// Find the paths for SAU

struct IEnvironment
{
    enum class InstallMode
    {
        ScheduledInstallMode = 0,
        ManualInstallMode,
        BootstrapInstallMode
    };

    virtual ~IEnvironment() = default;

//    virtual sophos::Logger::Level LogLevel() const = 0;
    virtual uint64_t LogBitmask() const = 0;

//    static inline REGSAM SoftwareSophosAutoUpdate_KeyFlags(REGSAM flags = 0)
//    {
//        // TODO: migrate to KEY_WOW64_64KEY on 64-bit intel!
//        return flags | CppCommon::WindowsOnWindows::keyWow;
//    }

//    virtual RegistryHandle CreateSoftwareKey(const std::string& key, REGSAM samDesired, DWORD options) const = 0;
//    // Open a key under [HKLM] SOFTWARE (or SOFTWARE\WOW6432Node)
//    virtual RegistryHandle OpenSoftwareKey(const std::string& key, REGSAM samDesired) const = 0;
//    // Open a key under [HKLM] SYSTEM
//    virtual RegistryHandle OpenSystemKey(const std::string& key, REGSAM samDesired) const = 0;
//
//    virtual HKEY GetRootKey() const = 0;

    // General-purpose paths
//    virtual std::experimental::filesystem::path GetTempDir() const = 0;
//    virtual std::experimental::filesystem::path GetWindowsDir() const = 0;
//    virtual std::experimental::filesystem::path GetSystemDir() const = 0;
//    virtual std::experimental::filesystem::path GetProgramDataDir() const = 0;
//    virtual std::experimental::filesystem::path GetSophosProgramDataDir() const = 0;
//    virtual std::experimental::filesystem::path GetProgramFilesNativeDir() const = 0;
//    virtual std::experimental::filesystem::path GetProgramFilesX86Dir() const = 0;
//#ifdef _M_ARM64
//    virtual std::experimental::filesystem::path GetProgramFilesArm32Dir() const = 0;
//#endif
//    virtual std::experimental::filesystem::path GetSophosProgramFilesNativeDir() const = 0;
//    virtual std::experimental::filesystem::path GetSophosProgramFilesX86Dir() const = 0;
//    virtual std::experimental::filesystem::path GetSophosProgramFilesArchDir() const = 0;
//
//    // AutoUpdate-specific paths (based on the above paths)
//    virtual std::experimental::filesystem::path GetConfigDir() const = 0;
//    virtual std::experimental::filesystem::path GetDataDir() const = 0;
//    virtual std::experimental::filesystem::path GetInstallDir() const = 0;
//    virtual std::experimental::filesystem::path GetDecodeDir() const = 0;
//    virtual std::experimental::filesystem::path GetExecDir() const = 0;
//    virtual std::experimental::filesystem::path GetWarehouseDir() const = 0;
//    virtual std::experimental::filesystem::path GetStatusDir() const = 0;
//    virtual std::experimental::filesystem::path GetMetricsDir() const = 0;
//    virtual std::experimental::filesystem::path GetLogDir() const = 0;
//    virtual std::experimental::filesystem::path GetCertificateDir() const = 0;
//    virtual std::experimental::filesystem::path GetSslCertificateDir() const = 0;
//    virtual std::experimental::filesystem::path GetEventsStagingDir() const = 0;
//    virtual std::experimental::filesystem::path GetEventsQueueDir() const = 0;
//    virtual std::experimental::filesystem::path GetEventsProcessedDir() const = 0;
//
//    // Empty if no local update source has been specified.
//    virtual std::experimental::filesystem::path GetLocalInstallSource() const = 0;

    virtual std::string GetSophosAlias() const = 0;

    virtual std::string GetMachineID() const = 0;
    virtual std::string GetMCSEndpointIdentity() const = 0;
    virtual std::string GetMCSCustomerIdentity() const = 0;

    virtual std::string GetSauVersion() const = 0;

    virtual CloudSubscriptions GetCloudSubscriptions() const = 0;
    virtual std::string GetCloudSubscriptionToken() const = 0;
    virtual bool HasCloudSubscriptionChanged() const = 0;

    virtual void EnsureQualityCompatKeyExists() const = 0;

    virtual std::set<std::string> GetPolicyFeatures() const = 0;
    virtual std::string GetPolicyFeaturesToken() const = 0;
    virtual bool HavePolicyFeaturesChanged() const = 0;
    virtual bool IsUsingDelayedSupplements() const = 0;

    virtual std::string GetCurrentPlatformToken() const = 0;
	virtual std::string GetCurrentPlatformRelease() const = 0;
    virtual bool IsServer() const = 0;
    virtual bool Is64BitOs() const = 0;
    virtual bool IsArm64() const = 0;
    virtual unsigned int OsMajorVersion() const = 0;
    virtual unsigned int OsMinorVersion() const = 0;
    virtual OSInfo::Data GetOsInfo() const = 0;

    // Has the operating system been upgraded since the last update?
    virtual bool IsUpgraded() const = 0;
    // Did the previous update succeed or fail?
    virtual bool IsLastUpgradeSuccessful() const = 0;

    virtual bool IsFlagSet(const std::string_view& flag) const = 0;

    virtual InstallMode GetInstallMode() const = 0;

    virtual bool HasNetworkConnectivity() const = 0;

    virtual std::set<ICacheEvaluator::CacheLocation> GetUpdateCaches() const = 0;

    virtual std::map<std::string, std::string> GetRegistryRedirects() const = 0;
    virtual std::vector<std::string> GetUpdateCacheRedirects() const = 0;

    virtual HealthEvents GetHealthEvents() const = 0;
    virtual void AddHealthEvent(const std::string& id, const std::string& familyId) = 0;
};
