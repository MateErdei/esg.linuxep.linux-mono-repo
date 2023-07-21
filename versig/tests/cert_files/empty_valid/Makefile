
all : signing.crt signing.key.pem rootca.crt inter.crt

.PRECIOUS :

CONFIG := ../openssl.cnf

%.key.pem :
	mkdir -p $(dir $@)
	openssl genrsa -out "$@" 4096

rootca.crt : rootca.key.pem
	openssl req -x509 -new -nodes \
		-config $(CONFIG) \
		-key $< \
		-days 10000 \
		-out $@ \
		-subj "/C=GB/ST=Oxfordshire/L=Abingdon/O=SAV for Linux/OU=Extra Files Signing/CN=Extra Files Signing/emailAddress=pair@santa"
	[ -s $@ ]

inter.csr : inter.key.pem
	openssl req -new \
	  -config $(CONFIG) \
	  -key "$<" \
	  -out "$@" \
	  -subj "/C=UK/ST=Oxford/L=Abingdon/O=SAVLinuxCloudAutomation/CN=inter"
	[ -s $@ ]

# inter.crt needs to be a CA otherwise we can't sign the signing cert
inter.crt : inter.csr rootca.crt
	openssl x509 \
	  -req -in $< \
	  -CA rootca.crt \
	  -CAkey rootca.key.pem \
	  -CAcreateserial \
	  -out "$@" \
	  -extensions v3_ca \
      -extfile $(CONFIG) \
      -days 9999 \
      -trustout
	[ -s $@ ]

signing.csr : signing.key.pem
	openssl req -new \
	  -config $(CONFIG) \
	  -key "$<" \
	  -out "$@" \
	  -subj "/C=UK/ST=Oxford/L=Abingdon/O=SAVLinuxCloudAutomation/CN=signing"
	[ -s $@ ]

signing.crt : signing.csr inter.crt inter.key.pem
	openssl x509 \
	  -req -in $< \
	  -CA inter.crt \
	  -CAkey inter.key.pem \
	  -CAcreateserial \
	  -out "$@" \
	  -extensions req_ext \
      -extfile $(CONFIG) \
      -days 9998
	[ -s $@ ]

clean :
	rm -f *.crt *.csr *.key.pem *.srl
