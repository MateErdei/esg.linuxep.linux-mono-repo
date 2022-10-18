
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

    std::shared_ptr<safestore::ISafeStoreWrapper> safeStoreWrapper = std::make_shared<safestore::SafeStoreWrapperImpl>();
    std::string safestoreDbDir = "/opt/sophos-spl/plugins/av/var/safestore_db/";
    std::string safestoreDbName = "safestore.db";
    std::string safestoreDbFile = Common::FileSystem::join(safestoreDbDir, safestoreDbName);
    std::string safestoreDbPwFile = Common::FileSystem::join(safestoreDbDir, "safestore.pw");

    auto fileSystem = Common::FileSystem::fileSystem();

    if (!fileSystem->isDirectory(safestoreDbDir))
    {
        std::cout << "SafeStore DB dir (" << safestoreDbDir << ") does not exist, is the product installed?"<< std::endl;;
        return 1;
    }

    if (!fileSystem->isFile(safestoreDbFile))
    {
        std::cout << "SafeStore DB file (" << safestoreDbFile << ") does not exist, did SafeStore initialise OK?"<< std::endl;;
        return 2;
    }

    if (!fileSystem->isFile(safestoreDbPwFile))
    {
        std::cout << "SafeStore DB password file (" << safestoreDbPwFile << ") does not exist."<< std::endl;;
        return 3;
    }

    std::string pw;
    try
    {
        pw = fileSystem->readFile(safestoreDbPwFile);
    }
    catch (const std::exception& ex)
    {
        std::cout << "Failed to read SafeStore password " << safestoreDbPwFile << ", " << ex.what() << std::endl;
        return 4;
    }


    auto initReturnCode = safeStoreWrapper->initialise(safestoreDbDir, safestoreDbName, pw);
    switch (initReturnCode)
    {
        case safestore::InitReturnCode::OK:
//            std::cout << "SafeStore DB connection initialised OK" << std::endl;
            break;
        case safestore::InitReturnCode::INVALID_ARG:

            break;
        case safestore::InitReturnCode::UNSUPPORTED_OS:

            break;
        case safestore::InitReturnCode::UNSUPPORTED_VERSION:

            break;
        case safestore::InitReturnCode::OUT_OF_MEMORY:

            break;
        case safestore::InitReturnCode::DB_OPEN_FAILED:

            break;
        case safestore::InitReturnCode::DB_ERROR:

            break;
        case safestore::InitReturnCode::FAILED:

            break;
    }

    printConfigOptions(safeStoreWrapper);

    safestore::SafeStoreFilter filter;
    filter.objectType = safestore::ObjectType::FILE;
    filter.activeFields = {safestore::FilterField::OBJECT_TYPE};

    for (auto& result : safeStoreWrapper->find(filter))
    {
        std::cout << "getObjectName: " << safeStoreWrapper->getObjectName(result) << std::endl;

        std::cout << "getObjectType: " ;
        switch (safeStoreWrapper->getObjectType(result))
        {
            case safestore::ObjectType::ANY:
                std::cout << "ANY" << std::endl;
                break;
            case safestore::ObjectType::FILE:
                std::cout << "FILE" << std::endl;
                break;
            case safestore::ObjectType::REGKEY:
                std::cout << "REGKEY" << std::endl;
                break;
            case safestore::ObjectType::REGVALUE:
                std::cout << "REGVALUE" << std::endl;
                break;
        }

        //        std::cout << "getObjectStatus: " << safeStoreWrapper->getObjectStatus(result) << std::endl;

        std::cout << "getObjectStoreTime: " << safeStoreWrapper->getObjectStoreTime(result) << std::endl;
        std::cout << "getObjectThreatId: " << safeStoreWrapper->getObjectThreatId(result) << std::endl;
        std::cout << "getObjectThreatName: " << safeStoreWrapper->getObjectThreatName(result) << std::endl;
        std::cout << "getObjectCustomDataString: " << safeStoreWrapper->getObjectCustomDataString(result, "SHA256") << std::endl;
    }

    return 0;
}