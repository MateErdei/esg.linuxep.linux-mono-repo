
TARGETS = \
	ca \
	server-fullchain.pem server-chain.pem server-private.pem server-private-chain.pem \
	server-out_of_date-private.pem \
	server-md5-private.pem \
	server-sha1-private.pem

all : $(TARGETS)

out-of-date : ca server-out_of_date-private.pem

md5 : ca server-md5-private.pem

sha1 : ca server-sha1-private.pem


.PHONY : all out-of-date md5 sha1

private/cakey.pem :
	mkdir -p $(dir $@)
	openssl genrsa -out "$@" 4096

root-ca.crt.pem : private/cakey.pem
	openssl req -x509 -new -nodes \
		-config ./openssl.cnf \
		-key private/cakey.pem \
		-days 3000 \
		-out $@ \
		-subj "/C=UK/ST=Oxford/O=SAVLinuxCloudAutomation/CN=localhostCA"
	[ -s $@ ]

localhost.privkey.pem :
	openssl genrsa \
	  -out "$@" \
	  4096
	[ -s $@ ]

localhost.csr.pem : localhost.privkey.pem
	openssl req -new \
	  -config ./openssl.cnf \
	  -key "$<" \
	  -out "$@" \
	  -subj "/C=UK/ST=Oxford/L=Abingdon/O=SAVLinuxCloudAutomation/CN=localhost"
	[ -s $@ ]

localhost.md5.csr.pem : localhost.privkey.pem
	which openssl
	openssl req -new \
	  -config ./openssl.cnf \
	  -key "$<" \
	  -out "$@" \
	  -md5 \
	  -subj "/C=UK/ST=Oxford/L=Abingdon/O=SAVLinuxCloudAutomation/CN=localhost"
	[ -s $@ ]

localhost.sha1.csr.pem : localhost.privkey.pem
	openssl req -new \
	  -config ./openssl.cnf \
	  -key "$<" \
	  -out "$@" \
	  -sha1 \
	  -subj "/C=UK/ST=Oxford/L=Abingdon/O=SAVLinuxCloudAutomation/CN=localhost"
	[ -s $@ ]

serial :
	echo 01 >"$@"

index.txt :
	touch "$@"

newcerts :
	mkdir -p newcerts

localhost.cert.pem : localhost.csr.pem root-ca.crt.pem private/cakey.pem
	openssl x509 \
	  -req -in localhost.csr.pem \
	  -CA root-ca.crt.pem \
	  -CAkey private/cakey.pem \
	  -CAcreateserial \
	  -out "$@" \
	  -extensions req_ext \
      -extfile openssl.cnf
	[ -s $@ ]

localhost.out_of_date.cert.pem : localhost.csr.pem root-ca.crt.pem private/cakey.pem | newcerts serial index.txt
	openssl ca \
		-batch \
		-verbose \
		-config ./openssl.cnf \
		-policy policy_anything \
		-out "$@" \
		-notext \
		-startdate 120815080000Z -enddate 120815090000Z \
		-cert root-ca.crt.pem \
		-keyfile private/cakey.pem \
		-infiles $< || { rm -f "$@" ; false ; }
	[ -s $@ ]

localhost.%.cert.pem : localhost.%.csr.pem root-ca.crt.pem private/cakey.pem | newcerts serial index.txt
	openssl ca \
		-batch \
		-verbose \
		-config ./openssl.cnf \
		-policy policy_anything \
		-out "$@" \
		-notext \
		-days 3 \
		-cert root-ca.crt.pem \
		-keyfile private/cakey.pem \
		-infiles $< || { rm -f "$@" ; false ; }
	[ -s $@ ]

server-private.pem : localhost.privkey.pem localhost.cert.pem
	cat $^ >$@

server-%-private.pem : localhost.privkey.pem localhost.%.cert.pem
	cat $^ >$@

server-chain.pem : root-ca.crt.pem
	cat $^ >$@

server-fullchain.pem : localhost.cert.pem root-ca.crt.pem
	cat $^ >$@

server-private-chain.pem : localhost.privkey.pem localhost.cert.pem root-ca.crt.pem
	cat $^ >$@

ca/root-ca.crt.pem : root-ca.crt.pem
	@mkdir -p ca
	cp $< $@

ca : ca/root-ca.crt.pem
	python c_rehash.py ca || c_rehash ca

.PHONY : ca

clean :
	rm -f *.pem serial* index.txt*
	rm -rf ca private newcerts root-ca.srl

.DELETE_ON_ERROR :

.SECONDARY :

