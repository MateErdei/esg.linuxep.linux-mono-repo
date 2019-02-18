/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

namespace Common
{
    namespace ObfuscationImpl
    {
        static const unsigned char CKeyDo_data[] = { 0x10, 0xd4, 0xfa, 0x82, 0xa0, 0x3a,
                                                     0x67, 0x90, 0xbe, 0x7d, 0x26, 0x2f };

        class CKeyDo
        {
        public:
            static const unsigned char* GetData() { return CKeyDo_data; }

            static size_t GetDataLen() { return sizeof(CKeyDo_data); }
        };
    } // namespace ObfuscationImpl
} // namespace Common
