// digest_buffer_checker.cpp: implementation of the
// digest_buffer_checker class.
//
//  20030902 Original code from DC version 1.0.0
//
//////////////////////////////////////////////////////////////////////

#include "digest_buffer_checker.h"

#include "crypto_utils.h"
#include "print.h"
#include "verify_exceptions.h"
#include "X509_certificate.h"

#include <algorithm>
#include <cassert>
#include <sstream>

namespace VerificationTool
{
    using namespace VerificationToolCrypto;
    using namespace verify_exceptions;
    using namespace manifest;

    bool digest_buffer_checker::verify_all(const std::vector<crypto::root_cert>& root_certificates, int allowed_algorithms)
    {
        auto all_sigs = signatures();
        decltype(all_sigs) sigs(all_sigs.size());

        if (all_sigs.empty())
        {
            throw std::runtime_error("Could not find any signatures: refusing to load unverified content");
        }

        // filter the signatures by the allowed_algorithms
        auto end = std::copy_if(
            all_sigs.begin(),
            all_sigs.end(),
            sigs.begin(),
            [&](const signature& sig) { return allowed_algorithms & sig.algo_; });
        sigs.resize(std::distance(sigs.begin(), end));

        if (sigs.empty())
        {
            throw std::runtime_error("No allowed signature found: refusing to load unverified content");
        }

        // sort the signatures by the algorithm strength, descending
        std::sort(sigs.begin(), sigs.end(), [](const signature &a, const signature &b) {
                      return b.algo_ < a.algo_;
                  });

        auto &body = file_body();
        int lastError = 0;
        std::string lastErrorMessage;
        for (const auto &signature : sigs)
        {
            try
            {
                crypto::X509_certificate x509(signature.certificate_);
                x509.verify_signature(body, signature);
                x509.verify_certificate_chain(signature.cert_chain_, root_certificates);
                // Break out of loop and return once there is a successful verification
                return true;
            }
            catch (const crypto::crypto_exception& ex)
            {
                lastError = 50;
                lastErrorMessage = ex.what();
                PRINT("Crypto exception: " << ex.what());
            }
            catch (const verify_exceptions::ve_badsig& ex)
            {
                lastError = ex.getErrorCode();
                PRINT("Bad signature: " << ex.getErrorCode());
            }
            catch (const verify_exceptions::ve_badcert& ex)
            {
                lastError = ex.getErrorCode();
                PRINT("Bad certificate: " << ex.what());
            }
            catch (const verify_exceptions::ve_crypt& ex)
            {
                lastError = ex.getErrorCode();
                lastErrorMessage = ex.message();
                PRINT("Openssl Error: " << lastErrorMessage);
            }
            catch (const verify_exceptions::ve_base& ex)
            {
                lastError = ex.getErrorCode();
                PRINT("Verification exception: " << ex.getErrorCode());
            }
            catch (const std::exception& e)
            {
                // Maybe don't throw
                PRINT(e.what());
            }
        }

        // If we've exhausted the certificates and still not validated the chain then throw
        switch (lastError)
        {
            case SignedFile::bad_signature:
                throw verify_exceptions::ve_badsig();
            case SignedFile::bad_certificate:
                throw verify_exceptions::ve_badcert();
            case SignedFile::openssl_error:
                throw verify_exceptions::ve_crypt(lastErrorMessage);
            case 50:
                throw crypto::crypto_exception(lastErrorMessage);
            default:
                throw verify_exceptions::ve_badcert();
        }
    }

} // namespace VerificationTool

// vim:ts=4
