// Copyright 2019-2023 Sophos Limited. All rights reserved.
#pragma once

#include "QueryResponse.h"
#include "ResponseDispatcher.h"

#include <memory>

namespace livequery
{
    class IQueryProcessor
    {
    public:
        virtual ~IQueryProcessor() = default;
        virtual QueryResponse query(const std::string& query) = 0;
        virtual std::unique_ptr<IQueryProcessor> clone() = 0;
    };

    int processQuery(
        livequery::IQueryProcessor& iQueryProcessor,
        livequery::IResponseDispatcher& dispatcher,
        const std::string& queryJson,
        const std::string& correlationId);
    const int INVALIDJSONERROR = 1;
    const int INVALIDREQUESTERROR = 2;
    const int INVALIDQUERYERROR = 3;
    const int FAILEDTOEXECUTEERROR = 4;
} // namespace livequery
