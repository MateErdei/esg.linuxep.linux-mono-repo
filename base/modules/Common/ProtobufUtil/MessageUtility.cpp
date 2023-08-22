// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "MessageUtility.h"

#include "MessageException.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

#include <google/protobuf/util/json_util.h>

#pragma GCC diagnostic pop

namespace Common::ProtobufUtil
{
    std::string MessageUtility::protoBuf2Json(const google::protobuf::Message& message)
    {
        using namespace google::protobuf::util;
        std::string json_output;
        JsonPrintOptions options;
        options.add_whitespace = true;
        auto status = MessageToJsonString(message, &json_output, options);
        if (!status.ok())
        {
            throw MessageException(status.ToString());
        }
        return json_output;
    }
} // namespace Common