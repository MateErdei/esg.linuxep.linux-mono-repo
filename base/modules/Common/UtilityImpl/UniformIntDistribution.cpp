/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "UniformIntDistribution.h"

namespace Common
{
    namespace UtilityImpl
    {


        UniformIntDistribution::UniformIntDistribution(int minValue, int maxValue)
                : m_uid(minValue, maxValue)
                  , m_engine(m_randomDevice())
        {

        }

        int UniformIntDistribution::next()
        {
            return m_uid(m_engine);
        }
    }
}