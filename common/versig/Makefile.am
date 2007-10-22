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
versig_CXXFLAGS = -isystem $(OPENSSL_INCLUDE) $(AM_CXXFLAGS) -DOPENSSL_NO_SHA512
versig_LDFLAGS = -L$(OPENSSL_LIBDIR) -Llib -lcrypto
versig_DEPENDENCIES = $(OPENSSL_LIBDIR)/libcrypto.so.$(OPENSSL_VERSION)

.libs/versig : versig


# Add in our extras.
bin_PROGRAMS += versig
