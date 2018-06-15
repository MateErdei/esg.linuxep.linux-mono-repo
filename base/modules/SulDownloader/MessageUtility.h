/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_MESSAGEUTILITY_H
#define EVEREST_BASE_MESSAGEUTILITY_H

#include <string>
#include <google/protobuf/util/json_util.h>

namespace SulDownloader
{

    class MessageUtility
    {
    public:
        static std::string protoBuf2Json( google::protobuf::Message & message);
    };

}

#endif //EVEREST_BASE_MESSAGEUTILITY_H
