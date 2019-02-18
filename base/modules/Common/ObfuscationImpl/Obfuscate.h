/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "SecureCollection.h"

// All of the ObsfucationImpl was adapted from //dev/EndInf/Blockbuster/ei/common/ in perforce.

namespace Common
{
    namespace ObfuscationImpl
    {
        Common::ObfuscationImpl::SecureString SECDeobfuscate(const std::string& srcData);
    }
} // namespace Common
