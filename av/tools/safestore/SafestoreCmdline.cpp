
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
    quarantineManager->deleteDatabase();
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
    std::string fakeVirusFilePath1 = "/tmp/fakevirus1";
    datatypes::AutoFd fd1; // just used to have something to pass in to QM
    std::string sha256PlaceHolder1 = "f2c91a583cfd1371a3085187aa5b2841ada3b62f5d1cc6b08bc02703ded3507a";
    std::string fakeVirusFilePath2 = "/tmp/fakevirus2";
    datatypes::AutoFd fd2; // just used to have something to pass in to QM
    std::string sha256PlaceHolder2 = "1cfc5ec228cebc49669fe56dba41a58a41e2b9d3b8c95528e47fa4f0c26c7169";
    fileSystem->writeFile(fakeVirusFilePath1, "a temp test file1");
    fileSystem->writeFile(fakeVirusFilePath2, "a temp test file2");
    // Store 1
    bool stored = quarantineManager->quarantineFile(fakeVirusFilePath1, "T_junk_threat_ID1", "threat name1", sha256PlaceHolder1, std::move(fd1));
    if (stored)
    {
        std::cout << "Successfully quarantined " << fakeVirusFilePath1 << std::endl;
    }
    else
    {
        std::cout << "Failed to quarantine " << fakeVirusFilePath1 << std::endl;
    }
    // Store 2
    stored = quarantineManager->quarantineFile(fakeVirusFilePath2,"T_junk_threat_ID2","threat name2", sha256PlaceHolder2, std::move(fd2));
    if (stored)
    {
        std::cout << "Successfully quarantined " << fakeVirusFilePath2 << std::endl;
    }
    else
    {
        std::cout << "Failed to quarantine " << fakeVirusFilePath2 << std::endl;
    }

    // Test searching

    safestore::SafeStoreFilter filter;
    filter.objectLocation = Common::FileSystem::dirName(fakeVirusFilePath1); // same dir as 2
//    filter.objectName = Common::FileSystem::basename(fakeVirusFilePath);
    filter.activeFields = {safestore::FilterField::OBJECT_LOCATION};

//    auto searchHandle = safeStoreWrapper->createSearchHandleHolder();
//    auto objectHandle  = safeStoreWrapper->createObjectHandleHolder();
//    safeStoreWrapper->findFirst(filter, *searchHandle, *objectHandle);
//
//    std::cout << "Object name: " << safeStoreWrapper->getObjectName(*objectHandle) << std::endl;

    for (auto& result : safeStoreWrapper->find(filter))
    {
        std::cout << "for loop: " << std::endl;
        std::cout << "getObjectName: " << safeStoreWrapper->getObjectName(result) << std::endl;
        std::cout << "getObjectId: " << safeStoreWrapper->getObjectId(result) << std::endl;
//        std::cout << "getObjectType: " << safeStoreWrapper->getObjectType(result) << std::endl;
//        std::cout << "getObjectStatus: " << safeStoreWrapper->getObjectStatus(result) << std::endl;
        std::cout << "getObjectStoreTime: " << safeStoreWrapper->getObjectStoreTime(result) << std::endl;
        std::cout << "getObjectThreatId: " << safeStoreWrapper->getObjectThreatId(result) << std::endl;
        std::cout << "getObjectThreatName: " << safeStoreWrapper->getObjectThreatName(result) << std::endl;
        std::cout << "getObjectCustomDataString: " << safeStoreWrapper->getObjectCustomDataString(result, "SHA256") << std::endl;
    }

//    // TODO 5675 fix const version
//    for (const auto& result : safeStoreWrapper->find(filter))
//    {
//        std::cout << "for loop: " << safeStoreWrapper->getObjectName(result) << std::endl;
//    }

    std::cout << "Done" << std::endl;

    // shutdown - implicitly called on safestore wrapper destructor so we don't need to worry about it.
}