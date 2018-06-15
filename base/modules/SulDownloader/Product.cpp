/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <cassert>
#include <tuple>
#include "Product.h"
#include "Logger.h"
#include "IFileSystem.h"
#include "Common/Process/IProcess.h"
#include "Common/Process/IProcessException.h"


namespace SulDownloader
{
    //enum class State{ Initialized, Distributed, Verified, Installed, HasError} m_state;
    Product::Product(ProductInformation productInformation)
            : m_state(State::Initialized), m_error(), m_productInformation(productInformation), m_distributePath(), m_productHasChanged(false)
    {

    }

    bool Product::verify()
    {
        assert(m_state == State::Distributed);
        m_state = State::Verified;
        // TODO: implement verify LINUXEP-6112
        return false;
    }


    void Product::install(const std::vector<std::string> & installArgs)
    {
        assert( m_state == State::Verified);
        m_state = State::Installed;

        auto fileSystem = ::Common::FileSystem::createFileSystem();

        std::string installShFile = fileSystem->join(m_distributePath, "install.shbroken");
        installShFile = "/tmp/installfail.sh";
        if(fileSystem->exists(installShFile) && !fileSystem->isDirectory(installShFile))
        {

            LOGINFO("Installing product: " << m_productInformation.getLine() << " version: " << m_productInformation.getVersion());

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
                LOGSUPPORT("Install exit code: " << exitCode);
                WarehouseError error;
                error.Description = std::string( "Product ") + getLine() + " failed to install";
                error.status = WarehouseStatus::INSTALLFAILED;
                setError(error);
            }
        }
//TODO set error for invalid install path. 
    }

    bool Product::hasError() const
    {
        return !m_error.Description.empty();
    }


    void Product::setError(WarehouseError error)
    {
        m_state = State::HasError;
        m_error = error;
    }


    WarehouseError Product::getError() const
    {
        return m_error;
    }

    std::string Product::distributionFolderName()
    {
        return m_productInformation.getLine() + m_productInformation.getVersion();
    }

    void Product::setDistributePath(const std::string &distributePath)
    {
        m_state = State::Distributed;
        m_distributePath = distributePath;
    }

    ProductInformation Product::getProductInformation()
    {
        return m_productInformation;
    }

    std::string Product::distributePath() const
    {
        return m_distributePath;
    }

    std::string Product::getLine() const
    {
        return m_productInformation.getLine();
    }

    std::string Product::getName() const
    {
        return m_productInformation.getName();
    }

    bool Product::productHasChanged() const
    {
        return m_productHasChanged;
    }

    void Product::setProductHasChanged(bool productHasChanged)
    {
        m_productHasChanged = productHasChanged;
    }

    std::string Product::getPostUpdateInstalledVersion() const
    {
        return m_postUpdateInstalledVersion;
    }

    void Product::setPostUpdateInstalledVersion(const std::string &postUpdateInstalledVersion)
    {
        m_postUpdateInstalledVersion = postUpdateInstalledVersion;
    }

    std::string Product::getPreUpdateInstalledVersion() const
    {
        return m_preUpdateInstalledVersion;
    }

    void Product::setPreUpdateInstalledVersion(const std::string &preUpdateInstalledVersion)
    {
        m_preUpdateInstalledVersion = preUpdateInstalledVersion;
    }




}
