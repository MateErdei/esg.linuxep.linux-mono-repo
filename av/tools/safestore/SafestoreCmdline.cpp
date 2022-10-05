
#include "safestore/IQuarantineManager.h"
#include "safestore/QuarantineManagerImpl.h"

//#include "Common/Logging/ConsoleFileLoggingSetup.h"
#include "safestore/SafeStoreWrapperImpl.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/Logging/ConsoleLoggingSetup.h"
#include "common/ApplicationPaths.h"

#include <filesystem>
#include <iostream>


void printConfigOptions(std::shared_ptr<safestore::ISafeStoreWrapper> safeStoreWrapper)
{
    // read config options:
    std::cout << "Config options: " << std::endl;
    if (auto autoPurge = safeStoreWrapper->getConfigIntValue(safestore::ConfigOption::AUTO_PURGE))
    {
        std::cout << "AUTO_PURGE: " << autoPurge.value() << std::endl;
    }

    if (auto maxObjSize = safeStoreWrapper->getConfigIntValue(safestore::ConfigOption::MAX_OBJECT_SIZE))
    {
        std::cout << "MAX_OBJECT_SIZE: " << maxObjSize.value() << std::endl;
    }

    if (auto maxObjInRegistrySubtree = safeStoreWrapper->getConfigIntValue(safestore::ConfigOption::MAX_REG_OBJECT_COUNT))
    {
        std::cout << "MAX_REG_OBJECT_COUNT: " << maxObjInRegistrySubtree.value() << std::endl;
    }

    if (auto maxSafeStoreSize = safeStoreWrapper->getConfigIntValue(safestore::ConfigOption::MAX_SAFESTORE_SIZE))
    {
        std::cout << "MAX_SAFESTORE_SIZE: " << maxSafeStoreSize.value() << std::endl;
    }

    if (auto maxObjCount = safeStoreWrapper->getConfigIntValue(safestore::ConfigOption::MAX_STORED_OBJECT_COUNT))
    {
        std::cout << "MAX_STORED_OBJECT_COUNT: " << maxObjCount.value() << std::endl;
    }
}

int main()
{
    Common::Logging::ConsoleLoggingSetup loggingSetup;


//    Common::Logging::PluginLoggingSetup setupFileLoggingWithPath(PLUGIN_NAME, "safestore");
    // PLUGIN_INSTALL
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
//    std::filesystem::path sophosInstall = appConfig.getData("SOPHOS_INSTALL");
//    std::filesystem::path pluginInstall = sophosInstall / "plugins" / "av";
    appConfig.setData("SOPHOS_INSTALL", "/tmp");
    appConfig.setData("PLUGIN_INSTALL", "/tmp/av");
    auto fileSystem = Common::FileSystem::fileSystem();
    fileSystem->makedirs(appConfig.getData("SOPHOS_INSTALL"));
    fileSystem->makedirs(appConfig.getData("PLUGIN_INSTALL"));
    fileSystem->makedirs( Plugin::getSafeStoreDbDirPath());

    // create QM and initialise it
    std::shared_ptr<safestore::ISafeStoreWrapper> safeStoreWrapper = std::make_shared<safestore::SafeStoreWrapperImpl>();
    std::shared_ptr<safestore::IQuarantineManager> quarantineManager = std::make_shared<safestore::QuarantineManagerImpl>(safeStoreWrapper);
    quarantineManager->initialise();


//    StateMonitor qmStateMonitorThread(quarantineManager);
//    qmStateMonitorThread.run();
//
    // read config options
    printConfigOptions(safeStoreWrapper);
    std::cout << std::endl;

    // write config options
    std::cout << "Writing config options: " << std::endl;
    if (safeStoreWrapper->setConfigIntValue(safestore::ConfigOption::AUTO_PURGE, false))
    {
        std::cout << "set AUTO_PURGE" << std::endl;
    }

    if (safeStoreWrapper->setConfigIntValue(safestore::ConfigOption::MAX_OBJECT_SIZE, 100000000000))
    {
        std::cout << "set MAX_OBJECT_SIZE" << std::endl;
    }

    if (safeStoreWrapper->setConfigIntValue(safestore::ConfigOption::MAX_REG_OBJECT_COUNT, 100))
    {
        std::cout << "set MAX_REG_OBJECT_COUNT" << std::endl;
    }

    if (safeStoreWrapper->setConfigIntValue(safestore::ConfigOption::MAX_SAFESTORE_SIZE, 200000000000))
    {
        std::cout << "set MAX_SAFESTORE_SIZE" << std::endl;
    }

    if (safeStoreWrapper->setConfigIntValue(safestore::ConfigOption::MAX_STORED_OBJECT_COUNT, 5000))
    {
        std::cout << "set MAX_STORED_OBJECT_COUNT" << std::endl;
    }
    std::cout << std::endl;

    // print config options again to show they're set
    printConfigOptions(safeStoreWrapper);
    std::cout << std::endl;

    // add some fake threats
    std::string fakeVirusFilePath = "/tmp/fakevirus";
    fileSystem->writeFile(fakeVirusFilePath, "a temp test file");
    bool stored = quarantineManager->quarantineFile(Common::FileSystem::dirName(fakeVirusFilePath), Common::FileSystem::basename(fakeVirusFilePath),"T_this_is_a_junk_threat_ID","threat name");
    if (stored)
    {
        std::cout << "Successfully quarantined " << fakeVirusFilePath << std::endl;
    }
    else
    {
        std::cout << "Failed to quarantine " << fakeVirusFilePath << std::endl;
    }
    // shutdown - implicitly called on safestore wrapper destructor so we don't need to worry about it.
}