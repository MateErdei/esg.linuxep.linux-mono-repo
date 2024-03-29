// Copyright 2003-2023 Sophos Limited. All rights reserved.

// signed_file.h: interface for the SignedFile facade class.
//
//  20030902 Original code from DC version 1.0.0
//  20050322 Modified for use with SAV for Linux

#if !defined(_SIGNEDFILE_H_INCLUDED_)
#    define _SIGNEDFILE_H_INCLUDED_

#include "Arguments.h"
#include "digest_buffer.h"
#include "digest_buffer_checker.h"

namespace VerificationTool
{
    class SignedFile
    {
    public:
        typedef enum
        {
            ok = 0,
            uninitialised,
            valid,
            notopened,
            malformed,
            openssl_error,
            bad_signature,
            bad_certificate,
            bad_syntax
        } status_enum;

        SignedFile();

        virtual ~SignedFile();

        //	void SetDigestSizeLimit
        //	(
        //		unsigned long MaxSizeBytes
        //	);
        //	//Sets the maximum digest buffer size

        virtual bool ReadBody() = 0;
        // Read body of signed file

        /**
         * Open the signed file and verify signature
         * @param arguments
         */
        void Open(const manifest::Arguments& arguments);

        bool IsValid();
        // Returns true if file properly signed

        // status_enum Status();
        // Returns status of signed-file

    protected:
        // Protected so can be used directly by child classes
        digest_buffer_checker m_DigestBuffer;
        status_enum m_Status;

    private:
        // unsigned long			m_DigestBufferMaxSizeBytes;	// 0 for default
    };

} // namespace VerificationTool

#endif // !defined(_SIGNEDFILE_H_INCLUDED_)
