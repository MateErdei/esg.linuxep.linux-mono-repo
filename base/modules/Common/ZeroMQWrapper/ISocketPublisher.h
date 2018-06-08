//
// Created by pair on 06/06/18.
//

#ifndef EVEREST_BASE_ISOCKETPUBLISHER_H
#define EVEREST_BASE_ISOCKETPUBLISHER_H

#include "IWritable.h"
#include "ISocketSetup.h"

namespace Common
{
    namespace ZeroMQWrapper
    {
        class ISocketPublisher : public virtual IWritable, public virtual ISocketSetup
        {
        public:
        };
    }
}

#endif //EVEREST_BASE_ISOCKETPUBLISHER_H
