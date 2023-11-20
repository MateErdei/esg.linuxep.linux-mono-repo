Generate new certs
run make all in base/testUtils/SupportFiles/https
cp server.private.pem to server.crt
cp ca/root-ca.crt.pem to server-root.crt
cp content of the private key into server.crt