// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/SslImpl/Digest.h"
#include "sophlib/crypto/Base64.h"

#include <nlohmann/json.hpp>
#include <string>

inline std::string generateJwtFromPayload(const std::string& payload)
{
    nlohmann::json header = nlohmann::json::object();
    header["typ"] = "JWT";
    header["alg"] = "RS384";
    header["x5c"] = nlohmann::json::array();
    const auto headerBase64 = sophlib::crypto::base64url::Encode(header.dump());

    const auto payloadBase64 = sophlib::crypto::base64url::Encode(payload);

    std::string signatureBase64 = sophlib::crypto::base64url::Encode("signature");

    return headerBase64 + "." + payloadBase64 + "." + signatureBase64;
}

inline std::string generateJwt(unsigned int size, const std::string& hash)
{
    nlohmann::json payload = nlohmann::json::object();
    payload["size"] = size;
    payload["sha256"] = hash;
    payload["iat"] = 0;
    payload["exp"] = 1;
    return generateJwtFromPayload(payload.dump());
}

inline std::string generateJwt(const std::string& body)
{
    return generateJwt(body.size(), Common::SslImpl::calculateDigest(Common::SslImpl::Digest::sha256, body));
}