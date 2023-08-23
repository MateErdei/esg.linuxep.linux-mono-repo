// Copyright 2022-2023 Sophos Limited. All rights reserved.
#pragma once

#include "Common/HttpRequests/IHttpRequester.h"

#include <map>
#include <memory>
#include <string>

namespace MCS
{
    class IAdapter
    {
    public:
        virtual ~IAdapter() = default;

        virtual std::string getStatusXml(std::map<std::string, std::string>& configOptions) const = 0;
        virtual std::string getStatusXml(
            std::map<std::string, std::string>& configOptions,
            const std::shared_ptr<Common::HttpRequests::IHttpRequester>&) const = 0;
    };
} // namespace MCS
