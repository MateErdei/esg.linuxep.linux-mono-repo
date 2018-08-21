/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <random>

namespace Common
{
    namespace UtilityImpl
    {

        class UniformIntDistribution
        {
        public:
            UniformIntDistribution(int minValue, int maxValue);

            int next();

        private:
            std::uniform_int_distribution<> m_uid;
            std::default_random_engine m_engine;
        };
    }
}



