// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include <string>
namespace safestore
{
    enum InitReturnCode
    {
        // QM initialised ok
        OK,

        // Failed to optne the threat storage database
        DB_OPEN_FAILED,

        // TODO add other failure cases here

        // QM failed to initialise for unknown reason
        FAILED
    };

    class IQuarantineManager
    {
    public:
        virtual ~IQuarantineManager() = default;

        /*
         * Initialise the Quarantine Manager and setup any resources needed.
         * Returns: InitReturnCode, to indicate if the operation was successful or not
         */
        virtual InitReturnCode initialise() = 0;


    };
}
