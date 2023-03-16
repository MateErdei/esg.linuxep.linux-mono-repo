// Copyright 2003-2023 Sophos Limited. All rights reserved.

// digest_buffer.cpp: implementation of the digest_file_buffer class.
//
//  20030902 Original code from DC version 1.0.0

#include "digest_buffer.h"

#include <istream>
#include <regex>

#include "hash.h"
#include "verify_exceptions.h"

using namespace std;
using namespace manifest;
using namespace crypto;
using namespace verify_exceptions;

// signed text file format / grammar
//
// Note this is a character level specification; there is no implied white space.
// This grammar should have the property that a single character change to the file
// should cause the parsing or verification to fail.
//
//	<digest file>		::= <digest file body><signature><certificate>+
//	<digest file body>	::= <any>*
//	<signature>		::= <signature header><signature body><signature footer>
//	<signature header>	::= "-----BEGIN SIGNATURE-----"<LF>
//	<signature body>	::= ((<base64>){32-79}<LF>){1-22}<equal>{0-2}<LF>
//	<signature footer>	::= "-----END SIGNATURE-----"<LF>
//	<certificate>		::= <certificate header><certificate body><certificate footer>
//	<certificate header>	::= "-----BEGIN CERTIFICATE-----"<LF>
//	<certificate body>	::= ((<base64>){32-79}<LF>){1-40}<equal>{0-2}<LF>
//	<certificate footer>	::= "-----END CERTIFICATE-----"<LF>
//
//	<space>	::= ' '
//	<dot>	::= '.'
//	<hash>	::= '#'
//	<quote>	::= '"'
//	<LF>	::= '\n'        (that is chr(10))
//	<equal>	::= '='
//      <any>   ::= .           (that is matches any single character)
//
//	<num>	   = [0-9]
//	<alphanum> = [0-9,a-z,A-Z]
//	<base64>   = [0-9,a-z,A-Z,'+','/']
//	<file>	   = [\32-\126,^'"',':','|','\'<','>','*','?']
//	<hex>	   = [0-9,a-f]
//
// The signature body specification means that it must consist of lines of base64 character
// between 32 and 79 columns wide, and that it must have between 1 and 40 lines. There may be
// 0 to two '=' characters at then end (part of the base64 encoding padding scheme).
// This specification ensure we make progress, ie do not simply accept an arbitrary number of
// line feed characters, but still allows some flexibility in line lengths and the size of key
// used to sign. 22 lines allows for signatures with rsa keys of length up to 8192 bit (which
// is very big, 2048 bit should be enough)
//

static const std::string g_begin_signature = "-----BEGIN SIGNATURE-----\n";
static const std::string g_end_signature = "-----END SIGNATURE-----\n";
static const std::string g_begin_certificate = "-----BEGIN CERTIFICATE-----\n";
static const std::string g_end_certificate = "-----END CERTIFICATE-----\n";
static std::regex g_extended_signature_regex(R"regex(#sig (\w+) ([A-Za-z0-9/+=]+)\n)regex");
static std::regex g_extended_signature_attribute_regex(R"regex(#\.\.\. (\w+) ([A-Za-z0-9/+=]+)\n)regex");

std::istream& manifest::operator>>(std::istream &in, digest_file_buffer &v)
{
    size_t body_size_prior_to_extensions = 0; // how much is verified by extended certs
    size_t body_size = 0;
    std::stringstream bodystrm;
    std::stringstream certstrm;
    signature sig{};
    enum { BODY, EXTENDED_SIGNATURES, TRAILER_SIGNATURE, TRAILER_CERTIFICATE } section = BODY;

    auto add_cert = [&sig](const std::string &cert) {
        if (sig.certificate_.empty())
            sig.certificate_ = cert;
        else
            sig.cert_chain_.push_back(cert);
    };
    auto add_sig = [&] {
        if (!sig.signature_.empty() && !sig.certificate_.empty())
        {
            sig.body_length_ = section == EXTENDED_SIGNATURES
                                   ? body_size_prior_to_extensions
                                   : body_size;
            v.signatures_.push_back(sig);
        }
        sig = signature{};
    };

    std::smatch match;
    std::vector<char> buffer(65536);
    auto buffer_size = static_cast<streamsize>(buffer.size());
    while (in.peek() != ios::traits_type::eof() && in.getline(&buffer[0], buffer_size))
    {
        auto size = static_cast<size_t>(in.gcount());
        if (!size) break;

        std::string line(&buffer[0], size);
        line[size - 1] = '\n';

        switch (section)
        {
            case BODY:
                if (line == g_begin_signature)
                    ; // fall through
                else if (std::regex_match(line, match, g_extended_signature_regex))
                    ; // and fall through
                else
                {
                    bodystrm << line;
                    body_size += size;
                    body_size_prior_to_extensions = body_size;
                    break;
                }

                section = EXTENDED_SIGNATURES;
                [[fallthrough]];

            case EXTENDED_SIGNATURES:
                if (line == g_begin_signature)
                {
                    add_sig();
                    sig.algo_ = crypto::ALGO_SHA1;
                    section = TRAILER_SIGNATURE;
                    break;
                }

                bodystrm << line;
                body_size += size;

                if (std::regex_match(line, match, g_extended_signature_regex))
                {
                    add_sig();
                    const auto & algo = match[1];
                    const auto & base64 = match[2];
                    if (crypto::parse_hash_algorithm(algo, sig.algo_))
                        sig.signature_ = base64;
                }
                else if (std::regex_match(line, match, g_extended_signature_attribute_regex))
                {
                    const auto & attr = match[1];
                    const auto & base64 = match[2];
                    if (attr == "cert")
                    {
                        auto cert = g_begin_certificate;
                        cert += std::string(base64);
                        cert += '\n';
                        cert += g_end_certificate;
                        add_cert(cert);
                    }
                }
                break;

            case TRAILER_SIGNATURE:
                if (line == g_end_signature)
                    section = TRAILER_CERTIFICATE;
                else
                    sig.signature_ += line;
                break;

            case TRAILER_CERTIFICATE:
                if (line == g_begin_certificate)
                    certstrm.str(std::string()); // reset certificate stream
                certstrm << line;
                if (line == g_end_certificate)
                    add_cert(certstrm.str());
                break;
        }
    }

    add_sig();
    v.file_buf_ = bodystrm.str();

    if (v.file_buf_.size() != body_size)
    {
        throw verify_exceptions::ve_badcert(); // we messed up!
    }

    if (v.signatures_.empty())
    {
        // No signing certificate
        throw verify_exceptions::ve_badcert();
    }

    return in;
}

