// Copyright 2003-2023 Sophos Limited. All rights reserved.

// digest_buffer_checker.h: interface for the digest_buffer_checker class.
//
//  20030902 Original code from DC version 1.0.0

#if !defined(_DIGEST_BUFFER_CHECKER_H_INCLUDED_)
#    define _DIGEST_BUFFER_CHECKER_H_INCLUDED_

#    include "digest_buffer.h"

#include "root_cert.h"

namespace VerificationTool
{
    using namespace std;

    class digest_buffer_checker : public digest_file_buffer
    {
    public:
        bool verify_all(const std::vector<crypto::root_cert>& root_certificates, int allowed_algorithms);
    };

} // namespace VerificationTool

#endif // !defined(_DIGEST_BUFFER_CHECKER_H_INCLUDED_)
