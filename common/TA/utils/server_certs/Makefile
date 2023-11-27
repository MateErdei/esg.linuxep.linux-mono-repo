root-ca.key.pem :
	openssl genrsa -out root-ca.key.pem 4096

server-root.crt : root-ca.key.pem
	openssl req -x509 -sha256 -new -nodes -key root-ca.key.pem \
		-days 3000 \
		-out $@ \
		-subj "/C=UK/ST=Oxford/O=SAVLinuxCloudAutomation/CN=localhostCA"

server.privkey.pem :
	openssl genrsa \
	  -out "$@" \
	  4096

server.csr.pem : server.privkey.pem
	openssl req -new \
	  -key "$<" \
	  -out "$@" \
	  -subj "/C=UK/ST=Oxford/L=Abingdon/O=SAVLinuxCloudAutomation/CN=localhost"

server.cert.pem : server.csr.pem server-root.crt root-ca.key.pem
	openssl x509 -sha256 \
	  -req -in server.csr.pem \
	  -CA server-root.crt \
	  -CAkey root-ca.key.pem \
	  -CAcreateserial \
	  -out "$@" \
	  -days 3000 \
	  -extensions req_ext \
	  -extfile openssl-ext.cnf

server.crt : server.privkey.pem server.cert.pem
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

utm.cert.pem : utm.csr.pem server-root.crt root-ca.key.pem
	openssl x509 -sha256 \
	  -req -in utm.csr.pem \
	  -CA server-root.crt \
	  -CAkey root-ca.key.pem \
	  -CAcreateserial \
	  -out "$@" \
	  -days 3000

clean :
	rm -f *.crt *.pem server-root.srl

all : server.crt utm.cert.pem
