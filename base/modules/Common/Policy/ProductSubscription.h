// Copyright 2023 Sophos All rights reserved.
#pragma once

#include <string>
#include <sstream>
#include <utility>

namespace Common::Policy
{
    constexpr char SSPLBaseName[] = "ServerProtectionLinux-Base";

    class ProductSubscription
    {
        public:
            ProductSubscription(
                std::string rigidName,
                std::string baseVersion,
                std::string tag,
                std::string fixedVersion,
                std::string esmVersionToken) :
                rigidName_(std::move(rigidName)),
                baseVersion_(std::move(baseVersion)),
                tag_(std::move(tag)),
                fixedVersion_(std::move(fixedVersion)),
                esmVersionToken_(std::move(esmVersionToken))
            {}

            ProductSubscription() = default;
            [[nodiscard]] const std::string& rigidName() const { return rigidName_; }
            [[nodiscard]] const std::string& baseVersion() const { return baseVersion_; }
            [[nodiscard]] const std::string& tag() const { return tag_; }
            [[nodiscard]] const std::string& fixedVersion() const { return fixedVersion_; }
            [[nodiscard]] const std::string& esmVersionToken() const { return esmVersionToken_; }

            [[nodiscard]] std::string toString() const
            {
                std::stringstream s;
                s << "name = " << rigidName_ << " baseversion = " << baseVersion_ << " tag = " << tag_
                  << " fixedversion = " << fixedVersion_ << " esmversion = " << esmVersionToken_;
                return s.str();
            }

            bool operator==(const ProductSubscription& rhs) const
            {
                return (
                    (rigidName_ == rhs.rigidName_) &&
                    (baseVersion_ == rhs.baseVersion_) &&
                    (tag_ == rhs.tag_) &&
                    (fixedVersion_ == rhs.fixedVersion_) &&
                    (esmVersionToken_ == rhs.esmVersionToken_));
            }

            bool operator!=(const ProductSubscription& rhs) const { return !operator==(rhs); }

        private:
            std::string rigidName_;
            std::string baseVersion_;
            std::string tag_;
            std::string fixedVersion_;
            std::string esmVersionToken_;
    };
}
