
all : data_files/manifest.dat.empty_valid data_files/manifest.dat.no_sha1

cert_files/empty_valid/signing.crt :
	make -C cert_files/empty_valid all

data_files/manifest.dat.empty_valid : cert_files/empty_valid/signing.crt
	python3 data_files/genManifest.py \
		--directory data_files/data_good \
		--output "$@" \
		--signing_key LOCAL_EMPTY \
		--sha1-signature

data_files/manifest.dat.no_sha1 : cert_files/empty_valid/signing.crt
	python3 data_files/genManifest.py \
		--directory data_files/data_good \
		--output "$@" \
		--signing_key LOCAL_EMPTY

