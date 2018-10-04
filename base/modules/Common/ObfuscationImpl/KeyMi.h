/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

namespace Common
{
    namespace ObfuscationImpl
    {
        static const unsigned char CKeyMi_data[] = {0x48, 0x48, 0x48, 0xc6, 0xc4, 0x78, 0x0, 0x14, 0xb3, 0xbf, 0x9f
                                                    , 0xfc, 0xce, 0x34, 0x40, 0x2c, 0x8, 0x10, 0x90, 0x5f, 0xfd, 0x1f
                                                    , 0x5b, 0xbf};

        class CKeyMi
        {
        public:
            static const unsigned char* GetData()
            {
                return CKeyMi_data;
            }

            static size_t GetDataLen()
            {
                return sizeof(CKeyMi_data);
            }
        };
    }

}
