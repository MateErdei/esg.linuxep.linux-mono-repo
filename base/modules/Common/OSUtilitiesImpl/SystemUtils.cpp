/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SystemUtils.h"

namespace OSUtilities
{
    std::string SystemUtilsImpl::getEnvironmentVariable(const std::string& key) const
    {
        auto val = std::getenv(key.c_str());
        if (val != nullptr)
        {
            return std::string(val);
        }
        return "";
    }

    std::unique_ptr<ISystemUtils>& systemUtilsStaticPointer()
    {
        static std::unique_ptr<ISystemUtils> instance =
                std::unique_ptr<ISystemUtils>(new SystemUtilsImpl());
        return instance;
    }

    ISystemUtils* systemUtils()
    {
        return systemUtilsStaticPointer().get();
    }
}
