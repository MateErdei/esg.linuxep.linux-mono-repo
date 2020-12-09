add_library(obfuscationimpl SHARED
        Cipher.h
        Cipher.cpp
        KeyDo.h
        KeyFa.h
        KeyMi.h
        KeyRa.h
        SecureCollection.h
        Obfuscate.h
        Obfuscate.cpp
        Obscurity.h
        Obscurity.cpp
        Base64.h
        Base64.cpp
        ../Obfuscation/IBase64Exception.h
        Logger.cpp
        Logger.h
        ../Obfuscation/IEvpCipherWrapper.h
        ../Obfuscation/IObfuscationException.h
        ../Obfuscation/ICipherException.h
        ../Obfuscation/IObscurityException.h
        )

target_include_directories(obfuscationimpl PUBLIC ${OPENSSL_INCLUDE_DIR} ${LOG4CPLUS_INCLUDE_DIR})

target_link_libraries(obfuscationimpl PUBLIC ${OPENSSLCRYPTO_LIBRARY})

SET_TARGET_PROPERTIES(obfuscationimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS obfuscationimpl
        LIBRARY DESTINATION files/base/lib64)
