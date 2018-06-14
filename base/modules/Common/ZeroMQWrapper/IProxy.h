///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#ifndef EVEREST_BASE_IPROXY_H
#define EVEREST_BASE_IPROXY_H

#include <string>
#include <memory>
#include "IContext.h"

namespace Common
{
    namespace ZeroMQWrapper
    {
        class IProxy
        {
        public:
            virtual ~IProxy() = default;
            virtual void start() = 0;
            virtual void stop() = 0;
        };

        using IProxyPtr = std::unique_ptr<IProxy>;

        extern IProxyPtr createProxy(const std::string& frontend, const std::string& backend);
    }
}


#endif //EVEREST_BASE_IPROXY_H
