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


versig_CXXFLAGS = -I$(OPENSSLDIR)/include $(AM_CXXFLAGS)
versig_LDFLAGS = -L$(OPENSSLDIR) -lcrypto
versig_DEPENDENCIES = $(OPENSSLDIR)/libcrypto.so.0.9.7

.libs/versig : versig


# Add in our extras.
bin_PROGRAMS += versig
