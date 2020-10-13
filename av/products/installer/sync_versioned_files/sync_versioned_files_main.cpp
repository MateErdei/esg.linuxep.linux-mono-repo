/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "sync_versioned_files.h"

#include "datatypes/Print.h"

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        PRINT("Syntax: sync_versioned_files <src> <dest>");
        return 2;
    }
    return sync_versioned_files::sync_versioned_files(argv[1], argv[2]);
}
