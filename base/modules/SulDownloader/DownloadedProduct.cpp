/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "DownloadedProduct.h"
#include "Logger.h"
#include "IFileSystem.h"
#include "Common/Process/IProcess.h"
#include "Common/Process/IProcessException.h"
#include "IVersig.h"
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include <cassert>
#include <cstring>



namespace SulDownloader
{
    DownloadedProduct::DownloadedProduct(const ProductMetadata& productInformation)
            : m_state(State::Initialized), m_error(), m_productMetadata(productInformation), m_distributePath(), m_productHasChanged(false)
    {

    }

    void DownloadedProduct::verify(const std::string & rootca )
    {
        assert(m_state == State::Distributed);
        m_state = State::Verified;
        auto iVersig = createVersig();
        auto fileSystem = ::Common::FileSystem::fileSystem();
        std::string product_path = fileSystem->join(m_distributePath,distributionFolderName());
        if ( iVersig->verify(rootca, product_path ) != IVersig::VerifySignature::SIGNATURE_VERIFIED)
        {
            WarehouseError error;
            error.Description = std::string( "Product ") + getLine() + " failed signature verification";
            error.status = WarehouseStatus::INSTALLFAILED;
            LOGERROR(error.Description);
            setError(error);
        }
        else
        {
            LOGINFO("Product verified: " << m_productMetadata.getLine());
        }
    }


    void DownloadedProduct::install(const std::vector<std::string> & installArgs)
    {
        assert( m_state == State::Verified);
        m_state = State::Installed;

        auto fileSystem = ::Common::FileSystem::fileSystem();

        std::string installShFile = fileSystem->join(distributionFolderName(), "install.sh");
        installShFile = fileSystem->join(m_distributePath, installShFile);

        if( fileSystem->exists(installShFile) && !fileSystem->isDirectory(installShFile) )
        {

            LOGINFO("Installing product: " << m_productMetadata.getLine() << " version: " << m_productMetadata.getVersion());
            LOGSUPPORT("Run installer: " << installShFile);

            fileSystem->makeExecutable(installShFile);

            auto process = ::Common::Process::createProcess();
            int exitCode =0;
            try
            {
                process->exec(installShFile, installArgs);
                auto output = process->output();
                LOGINFO(output);
                exitCode = process->exitCode();
            }
            catch ( Common::Process::IProcessException & ex)
            {
                LOGERROR(ex.what());
                exitCode = -1;
            }
            if ( exitCode!= 0 )
            {
                LOGERROR("Installation failed");
                LOGSUPPORT("Installer exit code: " << exitCode);
                LOGDEBUG("Possible reason: " << std::strerror(exitCode));
                if (exitCode == ENOEXEC)
                {
                    LOGERROR("Failed to run the installer. Hint: check first line starts with #!/bin/bash");
                }

                WarehouseError error;
                error.Description = std::string( "Product ") + getLine() + " failed to install";
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


    void DownloadedProduct::setError(const WarehouseError &error)
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

    void DownloadedProduct::setDistributePath(const std::string &distributePath)
    {
        m_state = State::Distributed;
        m_distributePath = distributePath;
    }

    const ProductMetadata & DownloadedProduct::getProductMetadata() const
    {
        return m_productMetadata;
    }

    const std::string& DownloadedProduct::distributePath() const
    {
        return m_distributePath;
    }

    const std::string&  DownloadedProduct::getLine() const
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

    const std::string& DownloadedProduct::getPostUpdateInstalledVersion() const
    {
        return m_postUpdateInstalledVersion;
    }

    void DownloadedProduct::setPostUpdateInstalledVersion(const std::string &postUpdateInstalledVersion)
    {
        m_postUpdateInstalledVersion = postUpdateInstalledVersion;
    }

    const std::string& DownloadedProduct::getPreUpdateInstalledVersion() const
    {
        return m_preUpdateInstalledVersion;
    }

    void DownloadedProduct::setPreUpdateInstalledVersion(const std::string &preUpdateInstalledVersion)
    {
        m_preUpdateInstalledVersion = preUpdateInstalledVersion;
    }




}
