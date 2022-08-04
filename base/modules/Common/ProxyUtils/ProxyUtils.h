/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <string>

#pragma once
namespace Common
    {
        class ProxyUtils
        {
        public:
            /*
            * Return URL and obfuscated credentials of current_proxy file.
             * This will throw if exceptions encountered while reading or parsing proxy file.
             */
            static std::tuple<std::string, std::string> getCurrentProxy();

            /*
            * Return URL and deobfuscated credentials of current_proxy file.
             * This will throw if exceptions encountered while deobfuscating creds
             */
            static std::tuple<std::string, std::string> deobfuscateProxyCreds(const std::string& creds);
        };
    }