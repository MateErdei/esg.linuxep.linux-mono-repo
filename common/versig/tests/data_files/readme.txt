
Regenerate these files with the Makefile:

manifest.dat.valid
   Original manifest.dat, valid for {tbp1.txt, tbp2.txt, tbp3.txt}

manifest.dat.spaces
   manifest.dat for {tbp1.txt, tbp 2.txt, tbp3.txt}

manifest.dat.badcert1
   Manifest.dat with bad certificate produced by modifying first byte of first certificate M -> N.

manifest.dat.badcert2
   Manifest.dat with bad certificate produced by modifying first byte of second certificate M -> N.

manifest.dat.badsig
   Manifest.dat with bad signature produced by modifying first byte of second line k -> l.

tbp2.txt.valid
   Original tbp2.txt (has same checksum to that recorded in manifest.dat)

tbp2.txt.modified
   Modified tbp2.txt (has different checksum to that recorded in manifest.dat)
