//
// Created by pair on 07/06/18.
//

#ifndef EVEREST_BASE_IREADWRITE_H
#define EVEREST_BASE_IREADWRITE_H

#include "IReadable.h"
#include "IWritable.h"

namespace Common
{
    namespace ZeroMQWrapper
    {
        class IReadWrite : public virtual IReadable, public virtual IWritable
        {
        };
    }
}
#endif //EVEREST_BASE_IREADWRITE_H
