// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "Common/HttpRequests/IHttpRequester.h"

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
            /*
             * Update request object with current proxy info, does not update creds if exception thrown when
             * deobfuscating creds
             */
            static bool updateHttpRequestWithProxyInfo(Common::HttpRequests::RequestConfig& request);
        };
    }