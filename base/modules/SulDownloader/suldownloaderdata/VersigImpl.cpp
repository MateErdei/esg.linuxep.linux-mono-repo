/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "VersigImpl.h"
#include "Common/Process/IProcess.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/Process/IProcessException.h"
#include "Logger.h"
namespace SulDownloader
{

    IVersig::VerifySignature
    VersigImpl::verify(const std::string& certificate_path, const std::string& productDirectoryPath) const
    {
        auto fileSystem = Common::FileSystem::fileSystem();

        if( !fileSystem->isFile(certificate_path))
        {
            LOGERROR("No certificate path to validate signature. Certificate provided does not exist: " << certificate_path);
            return VerifySignature::INVALID_ARGUMENTS;
        }

        if ( !fileSystem->isDirectory(productDirectoryPath))
        {
            LOGERROR("The given product path is not valid directory: " << productDirectoryPath);
            return VerifySignature::INVALID_ARGUMENTS;
        }


        std::string versig_path = Common::ApplicationConfiguration::applicationPathManager().getVersigPath();
        if ( !fileSystem->isExecutable(versig_path))
        {
            LOGERROR("Tool to verify signature can not be executed: " << versig_path);
            return VerifySignature::INVALID_ARGUMENTS;
        }

        std::string manifest_dat = Common::FileSystem::join(productDirectoryPath, "manifest.dat");
        if( !fileSystem->isFile(manifest_dat))
        {
            LOGERROR("No manifest.dat found. Path expected to be in: " << manifest_dat);
            return VerifySignature::INVALID_ARGUMENTS;
        }




        std::vector<std::string> versigArgs;
        versigArgs.emplace_back("-c"+certificate_path);
        versigArgs.emplace_back("-f"+manifest_dat);
        versigArgs.emplace_back("-d"+productDirectoryPath);
        versigArgs.emplace_back("--silent-off");

        auto process = ::Common::Process::createProcess();
        int exitCode =0;
        try
        {
            process->exec(versig_path, versigArgs);
            auto output = process->output();
            LOGSUPPORT(output);
            exitCode = process->exitCode();
        }
        catch ( Common::Process::IProcessException & ex)
        {
            LOGERROR(ex.what());
            exitCode = -1;
        }
        return exitCode == 0 ? VerifySignature::SIGNATURE_VERIFIED : VerifySignature::SIGNATURE_FAILED;
    }

    VersigFactory::VersigFactory()
    {
        restoreCreator();
    }

    VersigFactory& VersigFactory::instance()
    {
        static VersigFactory instance;
        return instance;
    }

    std::unique_ptr<IVersig> VersigFactory::createVersig()
    {
        return m_creator();
    }

    void VersigFactory::replaceCreator(std::function<std::unique_ptr<IVersig>(void)> creator)
    {
        m_creator = std::move(creator);
    }

    void VersigFactory::restoreCreator()
    {
        m_creator = [](){return std::unique_ptr<IVersig>(new VersigImpl());};
    }
    std::unique_ptr<IVersig> createVersig()
    {
        return VersigFactory::instance().createVersig();
    }


}