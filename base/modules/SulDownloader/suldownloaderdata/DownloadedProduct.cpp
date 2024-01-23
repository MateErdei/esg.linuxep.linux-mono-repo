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
    state_(State::Initialized),
    error_(),
    productMetadata_(productInformation),
    distributePath_(),
    productHasChanged_(false),
    productUninstall_(false),
    forceProductReinstall_(false),
    installFailureReason_("Update failed")
{
}

void DownloadedProduct::verify(const Common::Policy::UpdateSettings& updateSettings)
{
    assert(state_ == State::Distributed);
    state_ = State::Verified;
    auto iVersig = createVersig();

    if (iVersig->verify(updateSettings, distributePath_) != IVersig::VerifySignature::SIGNATURE_VERIFIED)
    {
        RepositoryError error;
        error.Description = std::string("Product ") + getLine() + " failed signature verification";
        error.status = RepositoryStatus::INSTALLFAILED;
        LOGERROR(error.Description);
        setError(error);
    }
    else
    {
        LOGINFO("Product verified: " << productMetadata_.getLine());
    }
}

void DownloadedProduct::install(const std::vector<std::string>& installArgs)
{
    assert(state_ == State::Verified);
    state_ = State::Installed;

    auto fileSystem = ::Common::FileSystem::fileSystem();

    std::string installShFile = installerPath();
    std::string productName = productMetadata_.getLine();
    std::string installOutputFile =
        Common::ApplicationConfiguration::applicationPathManager().getProductInstallLogFilePath(productName);

    if (fileSystem->exists(installShFile) && !fileSystem->isDirectory(installShFile))
    {
        LOGINFO("Installing product: " << productName << " version: " << productMetadata_.getVersion());
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
                    std::stringstream installFailureReason;
                    installFailureReason << productName << ": " << part;
                    installFailureReason_ = installFailureReason.str();
                    LOGERROR(installFailureReason.str());
                }
            }
            LOGERROR("See " << installOutputBasename << " for the full installer output.");
            // cppcheck-suppress shiftNegative
            LOGDEBUG("Installer exit code: " << exitCode);
            std::string installFailedOutputFile =
                    Common::ApplicationConfiguration::applicationPathManager().getProductInstallFailedLogFilePath(productName);
            try
            {
                fileSystem->writeFile(installFailedOutputFile, output);
            }
            catch (const Common::FileSystem::IFileSystemException& ex)
            {
                LOGWARN("Failed to write installer output to " << installFailedOutputFile << ": " << ex.what());
            }
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
    return !error_.Description.empty();
}

void DownloadedProduct::setError(const RepositoryError& error)
{
    state_ = State::HasError;
    error_ = error;
}

RepositoryError DownloadedProduct::getError() const
{
    return error_;
}

void DownloadedProduct::setDistributePath(const std::string& distributePath)
{
    state_ = State::Distributed;
    distributePath_ = distributePath;
}

const ProductMetadata& DownloadedProduct::getProductMetadata() const
{
    return productMetadata_;
}

void DownloadedProduct::setProductMetadata(ProductMetadata productMetadata)
{
    productMetadata_ = std::move(productMetadata);
}

const std::string& DownloadedProduct::distributePath() const
{
    return distributePath_;
}

const std::string& DownloadedProduct::getLine() const
{
    return productMetadata_.getLine();
}

bool DownloadedProduct::productHasChanged() const
{
    return productHasChanged_;
}

void DownloadedProduct::setProductHasChanged(bool productHasChanged)
{
    productHasChanged_ = productHasChanged;
}

bool DownloadedProduct::productWillBeDowngraded() const
{
    return productDowngrade_;
}

void DownloadedProduct::setProductWillBeDowngraded(bool willBeDowngraded)
{
    productDowngrade_ = willBeDowngraded;
}

void DownloadedProduct::setProductIsBeingUninstalled(bool IsBeingUninstalled)
{
    productUninstall_ = IsBeingUninstalled;
}

bool DownloadedProduct::getProductIsBeingUninstalled() const
{
    return productUninstall_;
}

void DownloadedProduct::setForceProductReinstall(bool forceReinstall)
{
    forceProductReinstall_ = forceReinstall;
}

bool DownloadedProduct::forceProductReinstall() const
{
    return forceProductReinstall_;
}

std::string DownloadedProduct::installerPath() const
{
    return Common::FileSystem::join(distributePath_, "install.sh");
}

std::string DownloadedProduct::getInstallFailureReason() const
{
    return installFailureReason_;
}
