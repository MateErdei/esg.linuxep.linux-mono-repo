
all : manifest.dat

serial :
	echo "01" >"$@"
	touch "$@"

index.txt :
	touch "$@"

rootca.key :
	openssl genrsa -out "$@" 2048

rootca.crt : rootca.key
	openssl req -new -x509 \
		-days 15000 \
		 -config openssl.cnf \
		 -extensions v3_ca \
		 -key "$<" -out "$@" \
		 -subj "/C=UK/ST=Oxford/O=VersigLongChain/CN=localhostCA"

rogue_intermediate.key :
	openssl genrsa -out "$@" 2048

rogue_intermediate.csr : rogue_intermediate.key
	openssl req -new -sha256 \
		-config openssl.cnf \
		-key "$<" \
		-out "$@" \
		-subj "/C=UK/ST=Oxford/O=VersigLongChain/CN=intermediate1"

rogue_intermediate.crt : rogue_intermediate.csr rootca.crt
	openssl x509 \
          -req -in "$<" \
          -extfile openssl.cnf \
          -extensions v3_intermediate_ca \
          -CA rootca.crt \
          -CAkey rootca.key \
          -CAcreateserial \
          -out "$@" \
          -days 14000


intermediate1.key :
	openssl genrsa -out "$@" 2048

intermediate1.csr : intermediate1.key
	openssl req -new -sha256 \
		-config openssl.cnf \
		-key "$<" \
		-out "$@" \
		-subj "/C=UK/ST=Oxford/O=VersigLongChain/CN=intermediate1"

intermediate1.crt : intermediate1.csr rootca.crt
	openssl x509 \
          -req -in "$<" \
          -extfile openssl.cnf \
          -extensions v3_intermediate_ca \
          -CA rootca.crt \
          -CAkey rootca.key \
          -CAcreateserial \
          -out "$@" \
          -days 14000


intermediate2.key :
	openssl genrsa -out "$@" 2048

intermediate2.csr : intermediate2.key
	openssl req -new -sha256 \
		-config openssl.cnf \
		-key "$<" \
		-out "$@" \
		-subj "/C=UK/ST=Oxford/O=VersigLongChain/CN=intermediate2"

# Deliberate error - using rogue_intermediate
intermediate2.crt : intermediate2.csr intermediate1.crt rogue_intermediate.crt
	openssl x509 \
          -req -in "$<" \
          -extfile openssl.cnf \
          -extensions v3_intermediate_ca \
          -CA rogue_intermediate.crt \
          -CAkey rogue_intermediate.key \
          -CAcreateserial \
          -out "$@" \
          -days 13000

intermediate3.key :
	openssl genrsa -out "$@" 2048

intermediate3.csr : intermediate3.key
	openssl req -new -sha256 \
		-config openssl.cnf \
		-key "$<" \
		-out "$@" \
		-subj "/C=UK/ST=Oxford/O=VersigLongChain/CN=intermediate3"

intermediate3.crt : intermediate3.csr intermediate2.crt
	openssl x509 \
          -req -in "$<" \
          -extfile openssl.cnf \
          -extensions v3_intermediate_ca \
          -CA intermediate2.crt \
          -CAkey intermediate2.key \
          -CAcreateserial \
          -out "$@" \
          -days 13000

signing.key :
	openssl genrsa -out "$@" 2048
	[ -s "$@" ]

signing.csr.pem : signing.key
	openssl req -new \
			-config openssl.cnf \
			-key "$<" \
			-out "$@" \
			-subj "/C=UK/ST=Oxford/L=Abingdon/O=VersigLongChain/CN=signing"
	[ -s "$@" ]

signing.crt : signing.csr.pem intermediate3.crt
	openssl x509 \
          -req -in "$<" \
          -CA intermediate3.crt \
          -CAkey intermediate3.key \
          -CAcreateserial \
          -out "$@" \
          -days 12000
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
	echo -n "#... cert " >> "$@"
	grep -v 'CERTIFICATE-----' intermediate3.crt | tr -d '\n' >>"$@"
	echo >>"$@"
	echo -n "#... cert " >> "$@"
	grep -v 'CERTIFICATE-----' intermediate2.crt | tr -d '\n' >>"$@"
	echo >>"$@"
	echo -n "#... cert " >> "$@"
	grep -v 'CERTIFICATE-----' intermediate1.crt | tr -d '\n' >>"$@"
	echo >>"$@"


clean :
	rm -f rootca.crt rootca.key
	rm -f rogue_intermediate.crt rogue_intermediate.csr rogue_intermediate.key
	rm -f intermediate1.crt intermediate1.csr intermediate1.key
	rm -f intermediate2.crt intermediate2.csr intermediate2.key
	rm -f intermediate3.crt intermediate3.csr intermediate3.key
	rm -f signing.crt signing.csr.pem signing.key
	rm -f manifest.dat manifest-sig.base64 manifest-sig.dat

.PHONY : clean all
