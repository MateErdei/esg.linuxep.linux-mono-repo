/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MessageUtility.h"
#include "MessageException.h"

#include <google/protobuf/util/json_util.h>


namespace Common
{
    namespace UtilityImpl
    {
        std::string MessageUtility::protoBuf2Json(const google::protobuf::Message &message)
        {
            using namespace google::protobuf::util;
            std::string json_output;
            JsonOptions options;
            options.add_whitespace = true;
            auto status = MessageToJsonString(message, &json_output, options);
            if (!status.ok())
            {
                throw MessageException(status.ToString());
            }
            return json_output;
        }
    }
}