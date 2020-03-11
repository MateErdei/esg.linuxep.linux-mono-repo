/******************************************************************************************************

Copyright 2019-2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
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

    void processQuery(
        livequery::IQueryProcessor& iQueryProcessor,
        livequery::IResponseDispatcher& dispatcher,
        const std::string& queryJson,
        const std::string& correlationId);
} // namespace livequery
