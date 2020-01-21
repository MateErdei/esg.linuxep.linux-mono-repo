/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "sophos_filesystem.h"
#include "IFileWalkCallbacks.h"

namespace filewalker
{
    void walk(sophos_filesystem::path starting_point, IFileWalkCallbacks& callbacks);
}
