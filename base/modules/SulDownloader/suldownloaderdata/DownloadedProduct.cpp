/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "DownloadedProduct.h"
#include "Logger.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/Process/IProcess.h"
#include "Common/Process/IProcessException.h"
#include "IVersig.h"
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include <cassert>
#include <cstring>


using namespace SulDownloader;
using namespace SulDownloader::suldownloaderdata;


DownloadedProduct::DownloadedProduct(const ProductMetadata& productInformation)
        : m_state(State::Initialized)
          , m_error()
          , m_productMetadata(productInformation)
          , m_distributePath()
          , m_productHasChanged(false)
          , m_productUninstall(false)
          , m_forceProductReinstall(false)
{

}

void DownloadedProduct::verify(const std::string& rootca)
{
    assert(m_state == State::Distributed);
    m_state = State::Verified;
    auto iVersig = createVersig();
    std::string product_path = Common::FileSystem::join(m_distributePath, distributionFolderName());
    if (iVersig->verify(rootca, product_path) != IVersig::VerifySignature::SIGNATURE_VERIFIED)
    {
        WarehouseError error;
        error.Description = std::string("Product ") + getLine() + " failed signature verification";
        error.status = WarehouseStatus::INSTALLFAILED;
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

    std::string installShFile = Common::FileSystem::join(m_distributePath, distributionFolderName(), "install.sh");

    if (fileSystem->exists(installShFile) && !fileSystem->isDirectory(installShFile))
    {

        LOGINFO("Installing product: " << m_productMetadata.getLine() << " version: "
                                       << m_productMetadata.getVersion());
        LOGSUPPORT("Run installer: " << installShFile);

        fileSystem->makeExecutable(installShFile);

        auto process = ::Common::Process::createProcess();
        int exitCode = 0;
        try
        {
            process->exec(installShFile, installArgs);
            auto output = process->output();
            LOGINFO(output);
            exitCode = process->exitCode();
        }
        catch (Common::Process::IProcessException& ex)
        {
            LOGERROR(ex.what());
            exitCode = -1;
        }
        if (exitCode != 0)
        {
            LOGERROR("Installation failed");
            LOGSUPPORT("Installer exit code: " << exitCode);
            LOGDEBUG("Possible reason: " << std::strerror(exitCode));
            if (exitCode == ENOEXEC)
            {
                LOGERROR("Failed to run the installer. Hint: check first line starts with #!/bin/bash");
            }

            WarehouseError error;
            error.Description = std::string("Product ") + getLine() + " failed to install";
            error.status = WarehouseStatus::INSTALLFAILED;
            setError(error);
        }
        else
        {
            LOGINFO("Product installed: " << m_productMetadata.getLine());
        }
    }
    else
    {
        LOGERROR("Invalid installer path: " << installShFile);
        WarehouseError error;
        error.Description = "Invalid installer";
        error.status = WarehouseStatus::INSTALLFAILED;
        setError(error);

    }
}

bool DownloadedProduct::hasError() const
{
    return !m_error.Description.empty();
}


void DownloadedProduct::setError(const WarehouseError& error)
{
    m_state = State::HasError;
    m_error = error;
}


WarehouseError DownloadedProduct::getError() const
{
    return m_error;
}

const std::string& DownloadedProduct::distributionFolderName()
{
    return m_productMetadata.getDefaultHomePath();
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
