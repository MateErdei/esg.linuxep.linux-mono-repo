
eicar.iso : eicar.com
	mkdir -p /tmp/iso/AAA
	mkdir -p /tmp/iso/DIR/subdir
	mkdir -p /tmp/iso/ZZZ
	echo clean >/tmp/iso/AAA/clean
	cp "$<" /tmp/iso/DIR/subdir/eicar.com
	echo clean >/tmp/iso/ZZZ/clean
	mkisofs -input-charset utf-8 -J -r -o $@ /tmp/iso
	rm -rf /tmp/iso

eicar-only.iso : eicar.com
	mkdir -p /tmp/iso/DIR/subdir
	cp "$<" /tmp/iso/DIR/subdir/eicar.com
	mkisofs -input-charset utf-8 -J -r -o $@ /tmp/iso
	rm -rf /tmp/iso
