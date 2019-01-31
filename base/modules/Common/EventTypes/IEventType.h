/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>
namespace Common
{
    namespace EventTypes
    {
        class IEventType
        {
        public:
            virtual ~IEventType() = default;

            /**
           * Gets the type of event as a string used to identify class to reconstruct.
           * @return the name of the eventtype e.g. Credentials
           */
            virtual std::string getEventTypeId() const = 0;

            /**
             * Converts event object to a capn byte string.
             * @return string of bytes representing capn object.
             */
            virtual std::string toString() const = 0;
        };
    }
}
