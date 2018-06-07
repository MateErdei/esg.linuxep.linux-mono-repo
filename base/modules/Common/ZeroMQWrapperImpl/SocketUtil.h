//
// Created by pair on 07/06/18.
//

#ifndef EVEREST_BASE_SOCKETUTIL_H
#define EVEREST_BASE_SOCKETUTIL_H


#include "SocketHolder.h"

#include <string>
#include <vector>

namespace Common
{
    namespace ZeroMQWrapperImpl
    {
        class SocketUtil
        {
        public:
            SocketUtil() = delete;

            static std::vector<std::string> read(SocketHolder&);

            static void write(SocketHolder&, const std::vector<std::string> &data);
        };
    }
}


#endif //EVEREST_BASE_SOCKETUTIL_H
