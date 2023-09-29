// Copyright 2023 Sophos All rights reserved.

#include "common/versig/crypto_utils.h"

#include <gtest/gtest.h>

// base64_decode

TEST(base64, helloworld)
{
    const auto B64="SGVsbG8gV29ybGQ=\n";
    auto decoded = VerificationToolCrypto::base64_decode(B64);
    EXPECT_EQ(decoded, "Hello World");
}

TEST(base64, decode)
{
    const auto B64="i9D/tuW3wG5tYDlw5Ta3zUWIXTCdrsS2Xr8uunAf7fy53sAVyUKsnlp8/B6PsnxQIEhjR+3Ee1cm9iQVCvY5C2UPKY29L+8Q1++tyrzM5HVh+rOLNKMpPjVUiWn2EtEKjdA86JgFWCSWTF5SASWwSP9GUA8TOboH4cB5A/m1xKL8ZR2UefjahDyOrYWJjifHy53jDZIwOkkdG2jQcIx9HrQv+BLkPyy6iXPdWlOPUbyBnGGmhYBghR48LfqtW/0iEL6JKFOfYLNrZms73iWWEtw1NhIOj+jElXrte4/HXGAW6SoKKwHbUMr4nFZIXZ2sKjaWtnNTiiJ350XoelTudZQarjmwTkkdf/TWNawil4YPwWUAOhV8PXSzhEYri3QQaLz3ZfSOLSqe1ejf5jumaZo40Kcg40UZqpN7+No2u/VDnXyntLO/pTqgGvra9X4Yc9kaMKsyBle7xbqkI4PQS0cBugYTifYLI6apnh/8WlTqibKhjWGEEsXBT+SLWImqVeab/l9ndwpWjNmCC5u+BT1RmUlDpOfUFYOCgbWWtVmt8IBgzWhVmggBr4QnnB7oiBfPRZiHYW0N3q1jJkPDWEArPmaGesZ2u26NvQneq5UqIlPl4fktw/PnU1aaVgVZ3TaBEURL642pLzOWURRezGMZk5vAuUvxYioBaMq8YR4=\n";
    auto decoded = VerificationToolCrypto::base64_decode(B64);
    EXPECT_FALSE(decoded.empty());
}
