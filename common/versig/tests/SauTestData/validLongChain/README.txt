This directory contains a "hacked" manifest.dat that we want to fail verification.

The manifest.dat is well-formed, so it can be parsed. It was signed using a
custom certificate issued by an unrelated CA hierarchy, but contains a
production intermediate CA matching the production root.crt.

Critically, we want to ensure that (a) we verify the signature using the
certificate; and (b) we verify the certificate all the way to the root.

Instructions for creating OpenSSL Keys/Certificates
===================================================

This file documents how to:

- create a new self-signed certificate (by creating and signing a CSR)
- create an RSA private key
- create a certificate signing request (CSR)
- sign the CSR

Create a new CA certificate
---------------------------

cat > hax0r-ca.req <<EOF
[ req ]
distinguished_name  = HaX0rCA
prompt              = no
utf8                = yes
string_mask         = utf8only

[ HaX0rCA ]
CN                  = ca.hax0r.com
O                   = Sophos
C                   = UK
ST                  = Oxfordshire
L                   = Oxford
EOF
openssl req -batch -config hax0r-ca.req -x509 -nodes -days 3650 -newkey rsa:2048 -keyout hax0r-ca.pkey -out hax0r-ca.cert

Create a new 2,048-bit RSA private key (unencrypted)
----------------------------------------------------

$ openssl genrsa -out hax0r-signer.pkey 2048

Generate a certificate signing request (CSR)
--------------------------------------------

$ cat > hax0r-signer.req <<EOF
[ req ]
distinguished_name  = HaX0rSigner
prompt              = no
utf8                = yes
string_mask         = utf8only

[ HaX0rSigner ]
CN                  = signer.hax0r.com
O                   = Sophos
C                   = UK
ST                  = Oxfordshire
L                   = Oxford
EOF

$ openssl req -batch -new -key hax0r-signer.pkey -out hax0r-signer.csr -config hax0r-signer.req

Generate a self-signed certificate using the CSR and Private Key
----------------------------------------------------------------

$ openssl ca -in hax0r-signer.csr -out hax0r-signer.cert -keyfile hax0r-ca.pkey -cert hax0r-ca.cert -config hax0r-ca.config

Validate that the certificate is what you expected
--------------------------------------------------

$ openssl x509 -in hax0r-ca.cert -text -noout

# vim: set tw=0:
