/* Copyright 2012 Sophos Limited. All rights reserved.
 * Sophos and Sophos Anti-Virus are registered trademarks of Sophos Limited.
 * All other product and company names mentioned are trademarks or registered
 * trademarks of their respective owners.
 */

#pragma once
#include "Thumbprint.h"
#include "Logger.h"

#include <vector>
#include <set>

class ComponentState
{
public:
	ComponentState(void);
	explicit ComponentState(const std::string& lineId);

	void SetLineId(const std::string& lineId);
	std::string GetLineId() const;

	void SetName(const std::string& name);
	std::string GetName() const;

	void SetLongName(const std::string& longName);
	std::string GetLongName() const;

	void SetDefaultHomeFolder(const std::string& defaultHomeFolder);
	std::string GetDefaultHomeFolder() const;

	void SetIncludeRoot();
	bool GetIncludeRoot() const;

	void SetInstallable(bool isInstallable);
	bool IsInstallable() const;

	void SetDownloadedVersion(const std::string& version);
	std::string GetDownloadedVersion() const;

	void SetInstalledVersion(const std::string& version);
	std::string GetInstalledVersion() const;

	void SetDownloadedThumbprint(const Thumbprint& thumbprint);
	Thumbprint GetDownloadedThumbprint() const;

	void SetInstalledThumbprint(const Thumbprint& thumbprint);
	Thumbprint GetInstalledThumbprint();

	void SetSubcomponents(const std::vector<std::string>& subcomponents);
	std::vector<std::string> GetSubcomponents() const;

    static Thumbprint FailedInstallThumbprint();

private:
	std::string m_lineId;
	std::string m_name;
	std::string m_longName;
	std::string m_defaultHomeFolder;
	bool m_includeRoot;

	bool m_installable;

	std::string m_downloadedVersion;
	Thumbprint m_downloadedThumbprint;

	std::string m_installedVersion;
	Thumbprint m_installedThumbprint;

	std::vector<std::string> m_subcomponents;
};
