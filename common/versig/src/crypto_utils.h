// crypto_utils.h: interface for the cryptographic verification
// functions.
//
//  20030902 Original code from DC version 1.0.0
//
//////////////////////////////////////////////////////////////////////

#if !defined(_CRYPTO_H_INCLUDED_)
#    define _CRYPTO_H_INCLUDED_

#    include <cstdio>
#    include <iostream>
#    include <list>
#    include <string>

// uses the OpenSSL library
#    include <openssl/err.h>
#    include <openssl/evp.h>
#    include <openssl/pem.h>
#    include <openssl/ssl.h>
#    include <openssl/x509_vfy.h>

namespace VerificationToolCrypto
{
    using namespace std;

    X509* X509_decode(const string&);

    string base64_decode(const string&);

    string sha1sum(istream&);
    string sha256sum(istream&);
    string sha384sum(istream&);
    string sha512sum(istream&);

    /**
     * Size of a SHA1 hash digest
     */
    unsigned int sha1size();
    /**
     * Size of a SHA-512 hash digest
     */
    unsigned int sha512size();

    // This function is used to build a human-readable
    // error string from a stor context during the verification
    // of a certificate chain.
    //
    string MakeErrString(X509_STORE_CTX* stor);

} // namespace VerificationToolCrypto

namespace crypto
{
    using hash_t = std::string;
    hash_t hex(const std::string & binary);


    class crypto_exception : public std::runtime_error
    {
    public:
        explicit crypto_exception(const std::string& message)
            : std::runtime_error(message + " error: " + std::to_string(errno))
        {}
    };

}

#endif // !defined(_CRYPTO_H_INCLUDED_)
