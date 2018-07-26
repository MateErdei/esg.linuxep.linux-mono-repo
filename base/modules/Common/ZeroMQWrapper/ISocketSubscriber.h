//
// Created by pair on 06/06/18.
//

#pragma once


#include "IReadable.h"
#include "ISocketSetup.h"

namespace Common
{
    namespace ZeroMQWrapper
    {
        class ISocketSubscriber : public virtual IReadable, public virtual ISocketSetup
        {
        public:
            virtual void subscribeTo(const std::string& subject) = 0;
        };
    }
}


