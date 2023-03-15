


all : manifest.dat

rootca.key :
	openssl genrsa -out "$@" 2048

rootca.crt : rootca.key
	faketime -f '-5y' \
		openssl req -x509 -new -nodes \
			-config ./openssl.cnf \
			-key "$<" \
			-days 9000 \
			-out "$@" \
			-subj "/C=UK/ST=Oxford/O=VersigOldCA/CN=localhostCA"
	[ -s "$@" ]


signing.key :
	openssl genrsa -out "$@" 2048
	[ -s "$@" ]

signing.csr.pem : signing.key
	faketime -f '-4y' \
		openssl req -new \
			-config ./openssl.cnf \
			-key "$<" \
			-out "$@" \
			-subj "/C=UK/ST=Oxford/L=Abingdon/O=VersignOldSigning/CN=localhost"
	[ -s "$@" ]

serial :
	echo 01 >"$@"

index.txt :
	touch "$@"

signing.crt : signing.csr.pem rootca.crt
	faketime -f '-4y' \
        openssl x509 \
          -req -in "$<" \
          -CA rootca.crt \
          -CAkey rootca.key \
          -CAcreateserial \
          -out "$@" \
          -days 90
	[ -s $@ ]

manifest-sig.dat : manifest-data.dat | signing.crt
	openssl dgst -sha384 -sign signing.key -out "$@" "$<"

manifest-sig.base64 : manifest-sig.dat
	openssl base64 -in "$<" | tr -d '\n' >"$@"
	echo >>"$@"

manifest.dat : manifest-sig.base64 manifest-data.dat
	cat manifest-data.dat >"$@"
	echo -n "#sig sha384 " >>"$@"
	cat manifest-sig.base64 >>"$@"
	echo -n "#... cert " >> "$@"
	grep -v 'CERTIFICATE-----' signing.crt | tr -d '\n' >>"$@"
	echo >>"$@"

clean :
	rm -f rootca.key rootca.crt signing.key signing.csr.pem signing.crt serial index.txt
	rm -f manifest.dat manifest-sig.dat manifest-sig.base64

.PHONY : clean all
