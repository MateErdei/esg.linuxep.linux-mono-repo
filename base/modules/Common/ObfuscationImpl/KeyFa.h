/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

namespace Common
{
    namespace ObfuscationImpl
    {
        static const unsigned char CKeyFa_data[] = { 0xa6, 0xb2, 0x17, 0x93, 0xc0, 0xf8,
                                                     0x8e, 0x7a, 0x62, 0xef, 0xe8, 0x1e };

        class CKeyFa
        {
        public:
            static const unsigned char* GetData() { return CKeyFa_data; }

            static size_t GetDataLen() { return sizeof(CKeyFa_data); }
        };

    } // namespace ObfuscationImpl
} // namespace Common
