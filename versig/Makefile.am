## Makefile.am for standalone executable versig

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

## openssl includes included using isystem to limit warnings and accept long long type

if COMPILER_IS_GCC
VERSIG_OPENSSL_INCLUDE := $(ISYSTEM) $(OPENSSL_INCLUDE)
else !COMPILER_IS_GCC
VERSIG_OPENSSL_INCLUDE := -I$(OPENSSL_INCLUDE)
endif !COMPILER_IS_GCC

versig_CXXFLAGS = $(VERSIG_OPENSSL_INCLUDE) $(EXE_CXX_COMPILE_FLAGS)
versig_LDFLAGS =  $(EXE_LD_FLAGS) -Llib -lcrypto
versig_DEPENDENCIES = $(OPENSSL_LIBCRYPTO) $(OPENSSL_LIBSSP)

.libs/versig : versig


## Add in our extras.
bin_PROGRAMS += versig
