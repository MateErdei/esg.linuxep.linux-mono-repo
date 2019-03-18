// digest_buffer.cpp: implementation of the digest_file_buffer class.
//
//  20030902 Original code from DC version 1.0.0
//
//////////////////////////////////////////////////////////////////////

#include "digest_buffer.h"

#include "iostr_utils.h"
#include "verify_exceptions.h"

namespace VerificationTool
{
    using namespace std;

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

    istream& operator>>(istream& in, digest_file_buffer& v)
    {
        string body;
        string signature;
        string certificate;

        in >> noskipws;

        // look for the signature header
        in >> get_upto(body, "\n-----BEGIN SIGNATURE-----\n") >> match_base64(signature) >>
            expect("-----END SIGNATURE-----\n");

        if (0 == signature.length())
        {
            // No signature
            throw verify_exceptions::ve_missingsig();
        }

        // directly after the signature we expect one or more certificates
        string cert_header = "-----BEGIN CERTIFICATE-----\n";
        string cert_footer = "-----END CERTIFICATE-----\n";

        in >> expect(cert_header) >> match_base64(certificate) >> expect(cert_footer);

        if (0 == certificate.length())
        {
            // No signing certificate
            throw verify_exceptions::ve_badcert();
        }

        list<string> cert_chain;
        while (in.peek() == '-')
        {
            string cert;
            in >> expect(cert_header) >> match_base64(cert) >> expect(cert_footer);
            std::ostringstream ost;
            ost << cert_header << cert << cert_footer;
            cert_chain.push_front(ost.str());
        }
        in >> expect_eof;

        if (in)
        {
            v._file_buf = body + "\n";
            v._signature = signature;
            v._certificate = cert_header + certificate + cert_footer;
            v._cert_chain = cert_chain;
        }

        return in;
    }

} // namespace VerificationTool
