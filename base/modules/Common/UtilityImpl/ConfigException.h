// Copyright 2019-2023 Sophos Limited. All rights reserved.
#pragma once

#include "Common/Exceptions/IException.h"

namespace Common
{
    namespace UtilityImpl
    {
        /**
         * Use this exception to indicate that a required configuration is wrong and there is no way to restore or run
         * the system without that configuration.
         */
        class ConfigException : public Common::Exceptions::IException
        {
        public:
            using Common::Exceptions::IException::IException;
            ConfigException(const std::string& where, const std::string& cause);
        };
    } // namespace UtilityImpl
} // namespace Common
