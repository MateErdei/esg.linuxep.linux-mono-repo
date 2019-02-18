/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once
#include "SecureCollection.h"

namespace Common
{
    namespace ObfuscationImpl
    {
        /**
         * This class implements the <i>IObscurity</i> interface.
         * Specifically, it uses an asymetric key to obscure the data, and
         * uses various techniques to obscure this key.
         */
        class CObscurity
        {
        protected:
            typedef struct
            {
                unsigned char* value;
                size_t len;
            } LenVal_t;

            /**
             * Get password;
             * @return structure containing password. Caller takes ownership
             */
            Common::ObfuscationImpl::SecureDynamicBuffer GetPassword() const;

        protected:
            // -- Simple obscurity algorithms -----

            /**
             * Joins data up from sections
             * @param sections of data to be joined
             * @param count number of sections to be joined.
             * @param obscuredDataLen obscured data len
             * @return original data. Caller owns the returned buffer.
             */
            static unsigned char* Join(const LenVal_t* sections, size_t count, size_t* obscuredDataLen);

            /**
             * This method reveals the given obscured data
             * @param data data to be un-obscured
             * @param dataLen length of data to be un-obscured
             * @param dataLen modified to contain length of un-obscured data in bytes.
             * @return un-obscured data. Caller is responsible for deallocating this memory when no longer required.
             */
            static unsigned char* R1(const unsigned char* obscuredData, size_t obscuredDataLen, size_t* dataLen);

            static unsigned char* R2(const unsigned char* obscuredData, size_t obscuredDataLen, size_t* dataLen);

            static unsigned char* R3(const unsigned char* obscuredData, size_t obscuredDataLen, size_t* dataLen);

        public:
            CObscurity();

            virtual ~CObscurity();

            /**
             * @see IObscurity::Reveal()
             */
            SecureString Reveal(const std::string& data) const;
        };
    } // namespace ObfuscationImpl
} // namespace Common
