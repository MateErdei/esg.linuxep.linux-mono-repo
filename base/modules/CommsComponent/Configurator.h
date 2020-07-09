/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

namespace CommsComponent
{
    class NullConfigurator
    {
        public: 
        void applyParentSecurityPolicy(){}
        void applyParentInit(){}
        void applyChildSecurityPolicy(){}
        void applyChildInit(){}
    };
}