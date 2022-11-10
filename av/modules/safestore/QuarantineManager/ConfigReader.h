// Copyright 2022, Sophos Limited.  All rights reserved.

#include "safestore/SafeStoreWrapper/ISafeStoreWrapper.h"

#include <map>
#include <memory>

namespace
{
    std::map<std::string, safestore::SafeStoreWrapper::ConfigOption> optionsMap {
        { "MaxSafeStoreSize", safestore::SafeStoreWrapper::ConfigOption::MAX_SAFESTORE_SIZE },
        { "MaxObjectSize", safestore::SafeStoreWrapper::ConfigOption::MAX_OBJECT_SIZE },
        { "MaxRegObjectCount", safestore::SafeStoreWrapper::ConfigOption::MAX_REG_OBJECT_COUNT },
        { "AutoPurge", safestore::SafeStoreWrapper::ConfigOption::AUTO_PURGE },
        { "MaxStoreObjectCount", safestore::SafeStoreWrapper::ConfigOption::MAX_STORED_OBJECT_COUNT },
    };
}

namespace safestore
{
    void parseConfig(safestore::SafeStoreWrapper::ISafeStoreWrapper& safeStore);
};
