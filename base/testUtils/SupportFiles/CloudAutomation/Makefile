root-ca.key.pem :
	openssl genrsa -out root-ca.key.pem 4096

root-ca.crt.pem : root-ca.key.pem
	openssl req -x509 -sha256 -new -nodes -key root-ca.key.pem \
		-days 3000 \
		-out $@ \
		-subj "/C=UK/ST=Oxford/O=SAVLinuxCloudAutomation/CN=localhostCA"

localhost.privkey.pem :
	openssl genrsa \
	  -out "$@" \
	  4096

localhost.csr.pem : localhost.privkey.pem
	openssl req -new \
	  -key "$<" \
	  -out "$@" \
	  -subj "/C=UK/ST=Oxford/L=Abingdon/O=SAVLinuxCloudAutomation/CN=localhost"

localhost.cert.pem : localhost.csr.pem root-ca.crt.pem root-ca.key.pem
	openssl x509 -sha256 \
	  -req -in localhost.csr.pem \
	  -CA root-ca.crt.pem \
	  -CAkey root-ca.key.pem \
	  -CAcreateserial \
	  -out "$@" \
	  -days 3

server-private.pem : localhost.privkey.pem localhost.cert.pem
	cat $^ >$@

server-chain.pem : root-ca.crt.pem
	cat $^ >$@

server-fullchain.pem : localhost.cert.pem root-ca.crt.pem
	cat $^ >$@


utm.privkey.pem :
	openssl genrsa \
	  -out "$@" \
	  4096

utm.csr.pem : utm.privkey.pem
	openssl req -new \
	  -key "$<" \
	  -out "$@" \
	  -subj "/C=UK/ST=Oxford/L=Abingdon/O=SAVLinuxCloudAutomation/CN=localhost"

utm.cert.pem : utm.csr.pem root-ca.crt.pem root-ca.key.pem
	openssl x509 -sha256 \
	  -req -in utm.csr.pem \
	  -CA root-ca.crt.pem \
	  -CAkey root-ca.key.pem \
	  -CAcreateserial \
	  -out "$@" \
	  -days 3

cleanCerts :
	rm -f utm.cert.pem server-fullchain.pem server-chain.pem server-private.pem localhost.cert.pem root-ca.crt.pem root-ca.srl

all : server-fullchain.pem server-chain.pem server-private.pem utm.cert.pem
