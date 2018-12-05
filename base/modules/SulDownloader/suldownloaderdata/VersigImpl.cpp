/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "VersigImpl.h"
#include "Common/Process/IProcess.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/Process/IProcessException.h"
#include "Logger.h"


using namespace SulDownloader::suldownloaderdata;

IVersig::VerifySignature
VersigImpl::verify(const ConfigurationData& configurationData, const std::string& productDirectoryPath) const
{
    std::string certificate_path = Common::FileSystem::join(configurationData.getCertificatePath(), "rootca.crt");
    auto fileSystem = Common::FileSystem::fileSystem();

    if (!fileSystem->isFile(certificate_path))
    {
        LOGERROR(
                "No certificate path to validate signature. Certificate provided does not exist: " << certificate_path);
        return VerifySignature::INVALID_ARGUMENTS;
    }

    if (!fileSystem->isDirectory(productDirectoryPath))
    {
        LOGERROR("The given product path is not valid directory: " << productDirectoryPath);
        return VerifySignature::INVALID_ARGUMENTS;
    }

    std::string versig_path = Common::ApplicationConfiguration::applicationPathManager().getVersigPath();
    if (!fileSystem->isExecutable(versig_path))
    {
        LOGERROR("Tool to verify signature can not be executed: " << versig_path);
        return VerifySignature::INVALID_ARGUMENTS;
    }

    auto manifestPaths = configurationData.getManifestNames();
    if (manifestPaths.size() == 0)
    {
        manifestPaths.emplace_back("manifest.dat");
    }

    int exitCode = -1;
    for (auto& relativeManifestPath : manifestPaths)
    {
        // Check each manifest is correct
        auto dir = Common::FileSystem::dirName(relativeManifestPath);
        auto manifestDirectory = Common::FileSystem::join(productDirectoryPath, dir);
        auto manifestPath = Common::FileSystem::join(productDirectoryPath, relativeManifestPath);
        if (!fileSystem->isFile(manifestPath))
        {
            LOGERROR("Manifest not found. Path expected to be in: " << manifestPath);
            return VerifySignature::INVALID_ARGUMENTS;
        }

        std::vector<std::string> versigArgs;
        versigArgs.emplace_back("-c" + certificate_path);
        versigArgs.emplace_back("-f" + manifestPath);
        versigArgs.emplace_back("-d" + manifestDirectory);
        versigArgs.emplace_back("--silent-off");

        auto process = ::Common::Process::createProcess();
        try
        {
            process->exec(versig_path, versigArgs);
            auto output = process->output();
            LOGSUPPORT(output);
            exitCode = process->exitCode();
        }
        catch (Common::Process::IProcessException& ex)
        {
            LOGERROR(ex.what());
            exitCode = -1;
        }
        // Stop as soon as we get a failure
        if (exitCode != 0)
        {
            break;
        }
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

IVersigPtr VersigFactory::createVersig()
{
    return m_creator();
}

void VersigFactory::replaceCreator(VersigCreatorFunc creator)
{
    m_creator = std::move(creator);
}

void VersigFactory::restoreCreator()
{
    m_creator = []() { return IVersigPtr(new VersigImpl()); };
}


/**
 * Implement factory function
 * @return
 */
IVersigPtr SulDownloader::suldownloaderdata::createVersig()
{
    return VersigFactory::instance().createVersig();
}