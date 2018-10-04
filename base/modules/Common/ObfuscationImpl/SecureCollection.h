/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <vector>
#include <type_traits>
#include <string>
#include <array>

namespace Common
{
    namespace ObfuscationImpl
    {
        /**
         * This creates an object based on a std library memory allocation that
         * will write over its contents in memory when destroyed. This is to stop
         * any secure information persisting in memory.
         * @tparam Collection
         */
        template<typename Collection>
        class SecureCollection
                : public Collection
        {
            //Disable destroying SecureCollection polymorphically
            void* operator new( std::size_t  ) = delete;
            void* operator new[](std::size_t ) = delete;
            void  operator delete(void * )     = delete;
            void  operator delete[](void * )   = delete;

        public:
            using type = typename std::remove_const<typename Collection::value_type>::type;
            using Collection::Collection;

            ~SecureCollection()
            {
                volatile type* p = const_cast<type*>(Collection::data());
                int size_to_remove = Collection::size();
                while (size_to_remove--)
                {
                    *p++ = type{};
                }
            }
        };

        using SecureDynamicBuffer = SecureCollection<std::vector<unsigned char>>;
        using SecureString        = SecureCollection<std::string>;
        template<size_t N> using SecureFixedBuffer   = SecureCollection<std::array<unsigned char, N>>;
    }
}
