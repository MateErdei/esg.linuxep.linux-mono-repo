// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "DownloadedProduct.h"

#include "IVersig.h"
#include "Logger.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/PluginRegistryImpl/PluginInfo.h"
#include "Common/Process/IProcess.h"
#include "Common/Process/IProcessException.h"
#include "Common/UtilityImpl/StrError.h"
#include "sophlib/string/StringUtils.h"

#include <cassert>
#include <cstring>

using namespace SulDownloader;
using namespace SulDownloader::suldownloaderdata;
using Common::DownloadReport::RepositoryStatus;

DownloadedProduct::DownloadedProduct(const ProductMetadata& productInformation) :
    m_state(State::Initialized),
    m_error(),
    m_productMetadata(productInformation),
    m_distributePath(),
    m_productHasChanged(false),
    m_productUninstall(false),
    m_forceProductReinstall(false)
{
}

void DownloadedProduct::verify(const Common::Policy::UpdateSettings& updateSettings)
{
    assert(m_state == State::Distributed);
    m_state = State::Verified;
    auto iVersig = createVersig();

    if (iVersig->verify(updateSettings, m_distributePath) != IVersig::VerifySignature::SIGNATURE_VERIFIED)
    {
        RepositoryError error;
        error.Description = std::string("Product ") + getLine() + " failed signature verification";
        error.status = RepositoryStatus::INSTALLFAILED;
        LOGERROR(error.Description);
        setError(error);
    }
    else
    {
        LOGINFO("Product verified: " << m_productMetadata.getLine());
    }
}

void DownloadedProduct::install(const std::vector<std::string>& installArgs)
{
    assert(m_state == State::Verified);
    m_state = State::Installed;

    auto fileSystem = ::Common::FileSystem::fileSystem();

    std::string installShFile = installerPath();
    std::string productName = m_productMetadata.getLine();
    std::string installOutputFile =
        Common::ApplicationConfiguration::applicationPathManager().getProductInstallLogFilePath(productName);

    if (fileSystem->exists(installShFile) && !fileSystem->isDirectory(installShFile))
    {
        LOGINFO("Installing product: " << productName << " version: " << m_productMetadata.getVersion());
        LOGDEBUG("Run installer: " << installShFile);

        fileSystem->makeExecutable(installShFile);

        // Add in the installation directory to the SOPHOS_INSTALL environment variable when running installers.
        Common::PluginRegistryImpl::PluginInfo::EnvPairs envVariables;
        envVariables.emplace_back(
            "SOPHOS_INSTALL", Common::ApplicationConfiguration::applicationPathManager().sophosInstall());

        auto process = ::Common::Process::createProcess();
        std::string output;
        int exitCode = 0;
        try
        {
            // Non-thininstaller SulDownloader will now run as root:sophos-spl-group, so run installers as root:root to
            // avoid breaking any assumptions they may have
            process->exec(installShFile, installArgs, envVariables, 0, 0);
            auto status = process->wait(Common::Process::Milliseconds(1000), 600);
            if (status != Common::Process::ProcessStatus::FINISHED)
            {
                LOGERROR("Timeout waiting for installer " << installShFile << " to finish");
                process->kill();
            }
            output = process->output();
            exitCode = process->exitCode();
            fileSystem->writeFile(installOutputFile, output);
            LOGDEBUG("Written " << productName << " installer output to " << installOutputFile);
        }
        catch (const Common::Process::IProcessException& ex)
        {
            LOGERROR("Failed to run installer with error: " << ex.what());
            exitCode = -1;
        }
        catch (const Common::FileSystem::IFileSystemException& ex)
        {
            LOGWARN("Failed to write installer output to " << installOutputFile << ": " << ex.what());
        }
        if (exitCode != 0)
        {
            const auto installOutputBasename = Common::FileSystem::basename(installOutputFile);
            LOGERROR("Installation of " << productName << " failed.");
            const auto parts = sophlib::string::Split(output, sophlib::string::Is('\n'));
            for (const auto& part : parts)
            {
                if (!part.empty() && !sophlib::string::StartsWith(part, "+"))
                {
                    LOGERROR(productName << ": " << part);
                }
            }
            LOGERROR("See " << installOutputBasename << " for the full installer output.");
            // cppcheck-suppress shiftNegative
            LOGDEBUG("Installer exit code: " << exitCode);
            LOGDEBUG("Possible reason: " << Common::UtilityImpl::StrError(exitCode));
            if (exitCode == ENOEXEC)
            {
                LOGERROR("Failed to run the installer. Hint: check first line starts with #!/bin/bash");
            }

            RepositoryError error;
            error.Description = std::string("Product ") + getLine() + " failed to install";
            error.status = RepositoryStatus::INSTALLFAILED;
            setError(error);
        }
        else
        {
            LOGINFO("Product installed: " << productName);
        }
    }
    else
    {
        LOGERROR("Invalid installer path: " << installShFile);
        RepositoryError error;
        error.Description = "Invalid installer";
        error.status = RepositoryStatus::INSTALLFAILED;
        setError(error);
    }
}

bool DownloadedProduct::hasError() const
{
    return !m_error.Description.empty();
}

void DownloadedProduct::setError(const RepositoryError& error)
{
    m_state = State::HasError;
    m_error = error;
}

RepositoryError DownloadedProduct::getError() const
{
    return m_error;
}

void DownloadedProduct::setDistributePath(const std::string& distributePath)
{
    m_state = State::Distributed;
    m_distributePath = distributePath;
}

const ProductMetadata& DownloadedProduct::getProductMetadata() const
{
    return m_productMetadata;
}

void DownloadedProduct::setProductMetadata(ProductMetadata productMetadata)
{
    m_productMetadata = std::move(productMetadata);
}

const std::string& DownloadedProduct::distributePath() const
{
    return m_distributePath;
}

const std::string& DownloadedProduct::getLine() const
{
    return m_productMetadata.getLine();
}

bool DownloadedProduct::productHasChanged() const
{
    return m_productHasChanged;
}

void DownloadedProduct::setProductHasChanged(bool productHasChanged)
{
    m_productHasChanged = productHasChanged;
}

bool DownloadedProduct::productWillBeDowngraded() const
{
    return m_productDowngrade;
}

void DownloadedProduct::setProductWillBeDowngraded(bool willBeDowngraded)
{
    m_productDowngrade = willBeDowngraded;
}

void DownloadedProduct::setProductIsBeingUninstalled(bool IsBeingUninstalled)
{
    m_productUninstall = IsBeingUninstalled;
}

bool DownloadedProduct::getProductIsBeingUninstalled() const
{
    return m_productUninstall;
}

void DownloadedProduct::setForceProductReinstall(bool forceReinstall)
{
    m_forceProductReinstall = forceReinstall;
}

bool DownloadedProduct::forceProductReinstall() const
{
    return m_forceProductReinstall;
}

std::string DownloadedProduct::installerPath() const
{
    return Common::FileSystem::join(m_distributePath, "install.sh");
}
