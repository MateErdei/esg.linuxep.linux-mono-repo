// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "Jwt.h"

#include "JwtVerificationException.h"
#include "Logger.h"

#include "Common/UtilityImpl/StringUtils.h"
#include "sophlib/crypto/Base64.h"
#include "sophlib/crypto/HashAlgorithm.h"
#include "sophlib/sdds3/SignedData.h"

namespace SulDownloader
{
    // Based on
    // esg.em.esg/products/windows_endpoint/protection/sau/SDDSDownloader/SDDS3DownloaderLogic.cpp:verify_and_load_jwt_payload
    nlohmann::json verifyAndLoadJwtPayload(const ISignatureVerifierWrapper& verifier, const std::string& jwt)
    {
        try
        {
            const auto parts = Common::UtilityImpl::StringUtils::splitString(jwt, ".");
            if (parts.size() != 3)
            {
                std::stringstream errorMessage;
                errorMessage << "JWT does not consist of three parts, parts found: " << parts.size();
                throw JwtVerificationException(LOCATION, errorMessage.str());
            }

            const auto& headerPart = parts[0];
            const auto& payloadPart = parts[1];
            const auto& signaturePart = parts[2];

            const auto headerString = sophlib::crypto::base64url::Decode(headerPart);
            LOGDEBUG("JWT header: " << headerString);

            auto header = nlohmann::json::parse(headerString);

            if (!header.contains("typ") || header["typ"].type() != nlohmann::json::value_t::string ||
                header["typ"] != "JWT")
            {
                throw JwtVerificationException(
                    LOCATION, "Invalid JWT header: bad 'typ' field, header: " + headerString);
            }
            if (!header.contains("alg") || header["alg"].type() != nlohmann::json::value_t::string ||
                header["alg"] != "RS384")
            {
                throw JwtVerificationException(
                    LOCATION, "Invalid JWT header: bad 'alg' field, header: " + headerString);
            }
            if (!header.contains("x5c") || header["x5c"].type() != nlohmann::json::value_t::array)
            {
                throw JwtVerificationException(
                    LOCATION, "Invalid JWT header: bad 'x5c' field, header: " + headerString);
            }

            sophlib::crypto::Signature signature{};
            signature.algo = sophlib::crypto::ALGO_SHA384;
            signature.sigdata = sophlib::crypto::base64::Encode(sophlib::crypto::base64url::Decode(signaturePart));
            LOGDEBUG("JWT signature: " << signature.sigdata);
            for (const auto& cert : header["x5c"])
            {
                std::string b64_encoded = cert;
                std::string pem_encoded =
                    sophlib::sdds3::certutil::cert_b2a(sophlib::crypto::base64::Decode(b64_encoded));
                if (signature.signing_certificate.empty())
                {
                    signature.signing_certificate = pem_encoded;
                }
                else
                {
                    signature.intermediate_certificates.push_back(pem_encoded);
                }
            }

            sophlib::crypto::SignedBlob signed_blob;
            signed_blob.blob_ = headerPart + "." + payloadPart;
            signed_blob.signatures_ = { signature };
            verifier.Verify(signed_blob, signature.algo);

            const auto payloadString = sophlib::crypto::base64url::Decode(payloadPart);
            LOGDEBUG("JWT payload: " << payloadString);
            auto payload = nlohmann::json::parse(payloadString);

            // The JWT itself is valid. It is now safe to trust the payload.
            return payload;
        }
        catch (const nlohmann::json::exception& ex)
        {
            std::throw_with_nested(JwtVerificationException(LOCATION, ex.what()));
        }
        // For the exception thrown by sophlib::crypto::base64::Decode
        catch (const std::runtime_error& ex)
        {
            std::throw_with_nested(JwtVerificationException(LOCATION, ex.what()));
        }
    }
} // namespace SulDownloader