/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_ISENSORDATACALLBACK_H
#define EVEREST_BASE_ISENSORDATACALLBACK_H
#include <string>
namespace Common
{
    namespace PluginApi
    {
        class ISensorDataCallback
        {
        public:
            virtual ~ISensorDataCallback() = default;
            virtual void receiveData(const std::string &key, const std::string &data) = 0;
        };
    }
}
#endif //EVEREST_BASE_ISENSORDATACALLBACK_H
