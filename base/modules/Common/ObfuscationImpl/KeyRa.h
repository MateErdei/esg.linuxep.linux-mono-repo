/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

namespace Common
{
    namespace ObfuscationImpl
    {
        static const unsigned char CKeyRa_data[] = {0xae, 0xae, 0xae, 0x9b, 0x99, 0xf1, 0xd, 0x19, 0x50, 0x5c, 0x7c
                                                    , 0x1c, 0x2e, 0x2
                                                    , 0x1a, 0x76, 0xc1, 0xd9, 0x59, 0x20, 0x82, 0x57, 0x10, 0xf4};

        class CKeyRa
        {
        public:
            static const unsigned char* GetData()
            {
                return CKeyRa_data;
            }

            static size_t GetDataLen()
            {
                return sizeof(CKeyRa_data);
            }
        };
    }
}
