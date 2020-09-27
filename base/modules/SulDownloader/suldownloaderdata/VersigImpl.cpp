/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "VersigImpl.h"

#include "Logger.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/Process/IProcess.h"
#include "Common/Process/IProcessException.h"

using namespace SulDownloader::suldownloaderdata;

std::vector<std::string> VersigImpl::getListOfManifestFileNames(
                const ConfigurationData& configurationData,
                const std::string& productDirectoryPath) const
{
    auto fileSystem = Common::FileSystem::fileSystem();

    auto manifestPaths = configurationData.getManifestNames();

    if (manifestPaths.size() == 0)
    {
        LOGINFO("DEBUG LOG using default name for manifest.dat");
        // a manifest.dat file should exits for each component.
        manifestPaths.emplace_back("manifest.dat");
    }

    // optional manifest files to validate if they exists.

    auto optionalManifestPaths = configurationData.getOptionalManifestNames();
    for(auto& relativeManifestPath : optionalManifestPaths)
    {

        auto dir = Common::FileSystem::dirName(relativeManifestPath);
        auto manifestDirectory = Common::FileSystem::join(productDirectoryPath, dir);
        auto manifestPath = Common::FileSystem::join(productDirectoryPath, relativeManifestPath);

        //todo Wellie 1939
        if (fileSystem->isFile(manifestPath))
        {
            LOGINFO("DEBUG LOG list of relativeManifestPath: " << manifestPath);
            manifestPaths.emplace_back(relativeManifestPath);
        }
        else
        {
            //supplements will attached to a product and get downloaded under the product root look for their
            //manifest one level below the product directory path
            auto optionalManifestDirs = fileSystem->listDirectories(productDirectoryPath);
            for(const auto& optManifestDir : optionalManifestDirs)
            {
                LOGINFO("DEBUG LOG listed direcory: " << optManifestDir);
                auto supplement_manifestPath = Common::FileSystem::join(optManifestDir, relativeManifestPath);
                if (fileSystem->isFile(supplement_manifestPath))
                {
                    LOGINFO("DEBUG LOG supplement_manifestPath: " << supplement_manifestPath << "relative supplement" << Common::FileSystem::join(Common::FileSystem::basename(optManifestDir), relativeManifestPath));
                    manifestPaths.emplace_back(Common::FileSystem::join(Common::FileSystem::basename(optManifestDir), relativeManifestPath));
                    break;
                }
            }
        }
    }

    return manifestPaths;

}

IVersig::VerifySignature VersigImpl::verify(
    const ConfigurationData& configurationData,
    const std::string& productDirectoryPath) const
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

    auto manifestPaths = getListOfManifestFileNames(configurationData, productDirectoryPath);

    int exitCode = -1;
    for (auto& relativeManifestPath : manifestPaths)
    {
        LOGINFO( "DEBUG LOG manifest path found :" << relativeManifestPath);
        // Check each manifest is correct
        auto dir = Common::FileSystem::dirName(relativeManifestPath);
        auto manifestDirectory = Common::FileSystem::join(productDirectoryPath, dir);
        auto manifestPath = Common::FileSystem::join(productDirectoryPath, relativeManifestPath);

        if (!fileSystem->isFile(manifestPath))
        {
            LOGERROR("Manifest not found. Path expected to be in: " << manifestPath);
            return VerifySignature::INVALID_ARGUMENTS;
        }

        LOGINFO( "DEBUG LOG verify args for directory :" << manifestDirectory << "manifest path : " << manifestPath);
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