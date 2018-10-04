/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

// All of the ObsfucationImpl was adapted from //dev/EndInf/Blockbuster/ei/common/ in perforce.

#include "Obscurity.h"
#include "Base64.h"

namespace Common
{
    namespace ObfuscationImpl
    {

        Common::ObfuscationImpl::SecureString SECDeobfuscate(const std::string& srcData)
        {
            std::string decoded64BinaryText = Base64::Decode(srcData);
            CObscurity obscurity;
            return obscurity.Reveal(decoded64BinaryText);
        }
    }
}
