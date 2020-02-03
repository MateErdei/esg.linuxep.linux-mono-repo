
Updating the intermediate certificate 2015-12-11
------------------------------------------------

[DLCL][DM]
Emailed Simon Harrison to ask how to do the update, but he's out.

We updated intercert.crt using the most recent ostia warehouse:
1) Go to ostia.eng.sophos/dev/
2) Sort by date
3) Go to most recent warehouse
4) Go to .../catalogue/
5) Open one of the warehouses
6) Find the name of the signature file
7) Open the signature file .../<filename>
8) Copy the <intermediate_cert> section


Hi,

Looks like maybe you used the same key to generate the intermediate certificate as was used previously?

So we can just replace the intermediate certificate with the new one, which we got from the most recent Ostia warehouse.

It seems to pass one of our regression tests anyway.

I'd still like to know the correct way to get the certificate and generate a new key, for when that expires (2024).

Thanks.

-----Original Message-----
From: Douglas Leeder 
Sent: 11 December 2015 10:31
To: Simon Harrison; Tim Rayment
Cc: Tim Pickersgill; Donna Miles; Mark Burdett
Subject: Updating the SAV Linux regression suite warehouse generation intermediate certificate and signing key

Hi Simon, Tim,

We generate warehouses in our regression suite, and to that end have an intermediate certificate and signing key in our regression suite:
//dev/savlinux/regression/supportFiles/SDDS-WH-GEN-V-2.0/Authentication/...

Last year (2014-12-22) Tim Pickersgill updated the certificate, which lasted till 2015-12-10. 
Last year (2014-01-10) Darren updated the certificate and key from the savlinux ones.
Previously (2011-04-15) I regenerated them using //depot/tools/engineering_certificates

//depot/tools/engineering_certificates is clearly out of date - the cacerts/Aesclealca.crt is the one that expired in 2014.

Unfortunately Tim can remember what he did, only that it was very difficult to find out what to do.

Please could you tell us how to update these certificates and keys?

Thanks.

Engineering certificates
------------------------

Root certificate expires 2013
Intermediate certificate expires 2014

Local signing certificate and key


Regenerating signing certificate
--------------------------------

( I think the master copy is at //depot/tools/engineering_certificates/... )


You need the eng certificates kit, I am sure it is available all over the place, I know there is a copy here \\uk-filer6.eng.sophos\Deployment\SDDS-backend-tools\eng_certs_kit\eng_certs.

1. Create the new signing certificate using the ca batch file found in the eng certificates kit.
   E.g. ca -newcert CertName aescleal 1000
   Will result in the new certificate called "CertName" with a lifespan of 1000 days being generated.
   During the generation process a password will be required, make a note of the password specified as it is required when decoding the key.

2. Decrypt the key using openssl.
   E.g. openssl rsa -in UserKeys\CertName.pem -out UserKeys\DecodedCertName.pem
   Will result in the generated CertName key being decoded and written to the "DecodedCertName.pem" file in the "UserKeys" folder.
   Note, you will be prompted to provide the password for the key during the decode process.

3. Copy the generated files from the "UserCerts" folder and "UserKeys" folder, in this case "UserCerts\CertName.crt" and "UserKeys\DecodedCertName.pem".


