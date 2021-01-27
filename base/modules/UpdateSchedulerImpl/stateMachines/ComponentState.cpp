#include "ComponentState.h"

ComponentState::ComponentState() :
    m_lineId(""),
    m_name(""),
    m_longName(""),
    m_includeRoot(false),
    m_installable(false),
    m_downloadedVersion(""),
    m_downloadedThumbprint(""),
    m_installedVersion(""),
    m_installedThumbprint("")
{
}

ComponentState::ComponentState(const std::string& lineId) :
    m_lineId(lineId),
    m_name(""),
    m_longName(""),
    m_includeRoot(false),
    m_installable(false),
    m_downloadedVersion(""),
    m_downloadedThumbprint(""),
    m_installedVersion(""),
    m_installedThumbprint("")
{
}

void ComponentState::SetLineId(const std::string& lineId)
{
    if (m_lineId != lineId)
    {
        m_lineId = lineId;
        LOGDEBUG("Set component line ID to " << lineId << ".");
    }
}

std::string ComponentState::GetLineId() const
{
    return m_lineId;
}

void ComponentState::SetName(const std::string& name)
{
    if (m_name != name)
    {
        m_name = name;
        LOGDEBUG("Set component name to " << name << ".");
    }
}

std::string ComponentState::GetName() const
{
    return m_name;
}


void ComponentState::SetLongName(const std::string& longName)
{
    if (m_longName != longName)
    {
        m_longName = longName;
        LOGDEBUG("Set component long name to " << longName << ".");
    }
}

std::string ComponentState::GetLongName() const
{
    return m_longName;
}

void ComponentState::SetDownloadedVersion(const std::string& version)
{
    if (m_downloadedVersion != version)
    {
        m_downloadedVersion = version;
        LOGDEBUG("Set component downloaded version to " << version << ".");
    }
}

std::string ComponentState::GetDownloadedVersion() const
{
    return m_downloadedVersion;
}

void ComponentState::SetInstalledVersion(const std::string& version)
{
    if (m_installedVersion != version)
    {
        m_installedVersion = version;
        LOGDEBUG("Set component installed version to " << version << ".");
    }
}

std::string ComponentState::GetInstalledVersion() const
{
    return m_installedVersion;
}

void ComponentState::SetDefaultHomeFolder(const std::string& defaultHomeFolder)
{
    m_defaultHomeFolder = defaultHomeFolder;
}

std::string ComponentState::GetDefaultHomeFolder() const
{
    return m_defaultHomeFolder;
}

void ComponentState::SetIncludeRoot()
{
    m_includeRoot = true;
}

bool ComponentState::GetIncludeRoot() const
{
    return m_includeRoot;
}

void ComponentState::SetInstallable(bool isInstallable)
{
    m_installable = isInstallable;
}

bool ComponentState::IsInstallable() const
{
    return m_installable;
}

void ComponentState::SetDownloadedThumbprint(const Thumbprint& thumbprint)
{
    if (m_downloadedThumbprint != thumbprint)
    {
        m_downloadedThumbprint = thumbprint;
        LOGDEBUG("Set downld thumbprint for " << m_lineId << " to " << thumbprint.GetThumbprint() << ".");
    }
}

Thumbprint ComponentState::GetDownloadedThumbprint() const
{
    return m_downloadedThumbprint;
}

void ComponentState::SetInstalledThumbprint(const Thumbprint& thumbprint)
{
    if (m_installedThumbprint != thumbprint)
    {
        m_installedThumbprint = thumbprint;
        LOGDEBUG("Set instld thumbprint for " << m_lineId << " to " << thumbprint.GetThumbprint() << ".");
    }
}

Thumbprint ComponentState::GetInstalledThumbprint()
{
    return m_installedThumbprint;
}

void ComponentState::SetSubcomponents(const std::vector<std::string>& subcomponents)
{
    m_subcomponents = subcomponents;
}

std::vector<std::string> ComponentState::GetSubcomponents() const
{
    return m_subcomponents;
}

Thumbprint ComponentState::FailedInstallThumbprint()
{
    return Thumbprint("failedInstall");
}