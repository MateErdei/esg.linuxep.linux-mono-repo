
manifest : manifest.dat.valid manifest.dat.spaces manifest.dat.badcert1 manifest.dat.badcert2 manifest.dat.badsig manifest.dat.nosha256

# Original manifest.dat, valid for {tbp1.txt, tbp2.txt, tbp3.txt}
manifest.dat.valid : tbp1.txt tbp2.txt tbp3.txt
	-chmod 600 $@
	python genManifest.py $@ $^

manifest.dat.nosha256 : tbp1.txt tbp2.txt tbp3.txt
	-chmod 600 $@
	SHA256=0 \
	    python genManifest.py $@ $^

# manifest.dat with {tbp1.txt, "tbp 2.txt", tbp3.txt}
manifest.dat.spaces : tbp1.txt tbp_2_with_spaces tbp3.txt
	-chmod 600 $@
	python genManifest.py $@ tbp1.txt "tbp 2.txt" tbp3.txt

# Manifest.dat with bad certificate produced by modifying first byte of first certificate M -> N.
manifest.dat.badcert1 : manifest.dat.valid
	-chmod 600 $@
	python corruptManifest.py $< $@ 1

# Manifest.dat with bad certificate produced by modifying first byte of second certificate M -> N.
manifest.dat.badcert2 : manifest.dat.valid
	-chmod 600 $@
	python corruptManifest.py $< $@ 2

# Manifest.dat with bad signature produced by modifying first byte of second line k -> l.
manifest.dat.badsig : manifest.dat.valid
	-chmod 600 $@
	python corruptManifest.py $< $@ S

# Original tbp2.txt (has same checksum to that recorded in manifest.dat)
tbp2.txt.valid :
	-chmod 600 $@
	echo "TestFile" >$@
	echo "--------" >>$@
	echo -n "TBP1-2" >>$@

# Modified tbp2.txt (has different checksum to that recorded in manifest.dat)
tbp2.txt.modified :
	-chmod 600 $@
	echo "TestFile" >$@
	echo "--------" >>$@
	echo "TBP1-x" >>$@

tbp%.txt : tbp%.txt.valid
	-chmod 600 $@
	cp $< $@

tbp_2_with_spaces : tbp2.txt.valid
	rm -f "tbp 2.txt"
	cp "$<" "tbp 2.txt"

.PHONY : tbp_2_with_spaces manifest
