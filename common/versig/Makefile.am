# Makefile.am for standalone executable versig

versig_SOURCES =	products/versig/CertificateTracker.cpp \
			products/versig/crypto_utils.cpp \
			products/versig/digest_body.cpp \
			products/versig/digest_body_checker.cpp \
			products/versig/digest_buffer.cpp \
			products/versig/digest_buffer_checker.cpp \
			products/versig/manifest_file.cpp \
			products/versig/signed_file.cpp \
			products/versig/verify_exceptions.cpp \
			products/versig/versig.cpp

# OPENSSL_NO_SHA512 defined for openssl >= 0.9.8 as it is not needed
# openssl includes included using isystem to limit warnings and accept long long type

if COMPILER_IS_GCC
VERSIG_OPENSSL_INCLUDE := -isystem $(OPENSSL_INCLUDE)
else !COMPILER_IS_GCC
VERSIG_OPENSSL_INCLUDE := -I$(OPENSSL_INCLUDE)
endif !COMPILER_IS_GCC

versig_CXXFLAGS = $(VERSIG_OPENSSL_INCLUDE) $(AM_CXXFLAGS) -DOPENSSL_NO_SHA512
versig_LDFLAGS = -Llib -lcrypto
versig_DEPENDENCIES = $(OPENSSL_LIBCRYPTO)

.libs/versig : versig


# Add in our extras.
bin_PROGRAMS += versig
