/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

// All of the ObsfucationImpl was adapted from //dev/EndInf/Blockbuster/ei/common/ in perforce.

#include "Base64.h"
#include "Obscurity.h"

namespace Common
{
    namespace ObfuscationImpl
    {
        Common::ObfuscationImpl::SecureString SECDeobfuscate(const std::string& srcData)
        {
            CObscurity obscurity;
            return obscurity.Reveal(Base64::Decode(srcData));
        }

        std::string SECObfuscate(std::string& password)
        {
            CObscurity obscurity;
            return Base64::Encode(obscurity.Conceal(password));
        }
    } // namespace ObfuscationImpl
} // namespace Common
