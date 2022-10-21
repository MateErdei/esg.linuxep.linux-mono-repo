// Copyright 2022, Sophos Limited.  All rights reserved.

// This binary will print the details of objects in the SafeStore database, useful for manual testing and TAP tests.

#include "safestore/QuarantineManager/IQuarantineManager.h"
#include "safestore/QuarantineManager/QuarantineManagerImpl.h"
#include "safestore/SafeStoreWrapper/SafeStoreWrapperImpl.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/Logging/ConsoleLoggingSetup.h"
#include "common/ApplicationPaths.h"

#include <iostream>

using namespace safestore::SafeStoreWrapper;

void printConfigOptions(std::shared_ptr<ISafeStoreWrapper> safeStoreWrapper)
{
    std::cout << "Config options: " << std::endl;
    if (auto autoPurge = safeStoreWrapper->getConfigIntValue(ConfigOption::AUTO_PURGE))
    {
        std::cout << "AUTO_PURGE: " << autoPurge.value() << std::endl;
    }

    if (auto maxObjSize = safeStoreWrapper->getConfigIntValue(ConfigOption::MAX_OBJECT_SIZE))
    {
        std::cout << "MAX_OBJECT_SIZE: " << maxObjSize.value() << std::endl;
    }

    if (auto maxObjInRegistrySubtree = safeStoreWrapper->getConfigIntValue(ConfigOption::MAX_REG_OBJECT_COUNT))
    {
        std::cout << "MAX_REG_OBJECT_COUNT: " << maxObjInRegistrySubtree.value() << std::endl;
    }

    if (auto maxSafeStoreSize = safeStoreWrapper->getConfigIntValue(ConfigOption::MAX_SAFESTORE_SIZE))
    {
        std::cout << "MAX_SAFESTORE_SIZE: " << maxSafeStoreSize.value() << std::endl;
    }

    if (auto maxObjCount = safeStoreWrapper->getConfigIntValue(ConfigOption::MAX_STORED_OBJECT_COUNT))
    {
        std::cout << "MAX_STORED_OBJECT_COUNT: " << maxObjCount.value() << std::endl;
    }
}

int main()
{
    Common::Logging::ConsoleLoggingSetup loggingSetup;

    // Safestore wrapper to interact with SafeStore database.
    std::shared_ptr<ISafeStoreWrapper> safeStoreWrapper = std::make_shared<SafeStoreWrapperImpl>();

    // Expected locations of safestore database in a product installation.
    std::string safestoreDbDir = "/opt/sophos-spl/plugins/av/var/safestore_db/";
    std::string safestoreDbName = "safestore.db";
    std::string safestoreDbFile = Common::FileSystem::join(safestoreDbDir, safestoreDbName);
    std::string safestoreDbPwFile = Common::FileSystem::join(safestoreDbDir, "safestore.pw");

    auto fileSystem = Common::FileSystem::fileSystem();

    // Check SafeStore dir exists.
    if (!fileSystem->isDirectory(safestoreDbDir))
    {
        std::cout << "SafeStore DB dir (" << safestoreDbDir << ") does not exist, is the product installed?"<< std::endl;;
        return 1;
    }

    // Check SafeStore database file exists.
    if (!fileSystem->isFile(safestoreDbFile))
    {
        std::cout << "SafeStore DB file (" << safestoreDbFile << ") does not exist, did SafeStore initialise OK?"<< std::endl;;
        return 2;
    }

    // Check SafeStore password file exists.
    if (!fileSystem->isFile(safestoreDbPwFile))
    {
        std::cout << "SafeStore DB password file (" << safestoreDbPwFile << ") does not exist."<< std::endl;;
        return 3;
    }

    // Read SafeStore password.
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
        case InitReturnCode::OK:
            break;
        case InitReturnCode::INVALID_ARG:
            std::cout << "ERROR - got INVALID_ARG when initialising SafeStore database connection" << std::endl;
            break;
        case InitReturnCode::UNSUPPORTED_OS:
            std::cout << "ERROR - got UNSUPPORTED_OS when initialising SafeStore database connection" << std::endl;
            break;
        case InitReturnCode::UNSUPPORTED_VERSION:
            std::cout << "ERROR - got UNSUPPORTED_VERSION when initialising SafeStore database connection" << std::endl;
            break;
        case InitReturnCode::OUT_OF_MEMORY:
            std::cout << "ERROR - got OUT_OF_MEMORY when initialising SafeStore database connection" << std::endl;
            break;
        case InitReturnCode::DB_OPEN_FAILED:
            std::cout << "ERROR - got DB_OPEN_FAILED when initialising SafeStore database connection" << std::endl;
            break;
        case InitReturnCode::DB_ERROR:
            std::cout << "ERROR - got DB_ERROR when initialising SafeStore database connection" << std::endl;
            break;
        case InitReturnCode::FAILED:
            std::cout << "ERROR - got FAILED when initialising SafeStore database connection" << std::endl;
            break;
    }

    // Print all current config options
    std::cout << "--- SafeStore database config options ---" << std::endl;
    printConfigOptions(safeStoreWrapper);
    std::cout << std::endl;

    // Create filter to find all files in SafeStore database
    SafeStoreFilter filter;
    filter.objectType = ObjectType::FILE;
    filter.activeFields = {FilterField::OBJECT_TYPE};

    std::cout << "--- SafeStore File Objects ---" << std::endl;
    // Perform search to get all objects and then print object details
    for (auto& result : safeStoreWrapper->find(filter))
    {
        std::cout << "Object Name: " << safeStoreWrapper->getObjectName(result) << std::endl;

        std::cout << "Object Type: " ;
        switch (safeStoreWrapper->getObjectType(result))
        {
            case ObjectType::ANY:
                std::cout << "ANY" << std::endl;
                break;
            case ObjectType::FILE:
                std::cout << "FILE" << std::endl;
                break;
            case ObjectType::REGKEY:
                std::cout << "REGKEY" << std::endl;
                break;
            case ObjectType::REGVALUE:
                std::cout << "REGVALUE" << std::endl;
                break;
        }

        std::cout << "Object Status: ";
        switch (safeStoreWrapper->getObjectStatus(result))
        {
            case ObjectStatus::ANY:
                std::cout << "ANY" << std::endl;
                break;
            case ObjectStatus::STORED:
                std::cout << "STORED" << std::endl;
                break;
            case ObjectStatus::QUARANTINED:
                std::cout << "QUARANTINED" << std::endl;
                break;
            case ObjectStatus::RESTORE_FAILED:
                std::cout << "RESTORE_FAILED" << std::endl;
                break;
            case ObjectStatus::RESTORED_AS:
                std::cout << "RESTORED_AS" << std::endl;
                break;
            case ObjectStatus::RESTORED:
                std::cout << "RESTORED" << std::endl;
                break;
        }

        std::cout << "Object Store Time: " << safeStoreWrapper->getObjectStoreTime(result) << std::endl;
        std::cout << "Object Threat ID: " << safeStoreWrapper->getObjectThreatId(result) << std::endl;
        std::cout << "Object Threat Name: " << safeStoreWrapper->getObjectThreatName(result) << std::endl;
        std::cout << "Object Custom Data (SHA256): " << safeStoreWrapper->getObjectCustomDataString(result, "SHA256") << std::endl;
        std::cout << std::endl;
    }

    return 0;
}