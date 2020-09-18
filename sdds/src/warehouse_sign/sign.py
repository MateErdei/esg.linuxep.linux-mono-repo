import hashlib
import os
import subprocess
import sys

help = """
WAREHOUSE_SIGN - utility for warehouse_sign warehouse files

Usage: warehouse_sign.py <file to sign> <info file>

<file to sign> is the file to create signature for.  The signature itself is placed
in a file with content-derived name, in the same directory as <file to sign> and that 
file name (without the extension) is then written to <info file>, to be read back by 
the caller.
"""


def Usage():
    print(help)
    exit(1)


def get_certificate(cert: str, key: str = 'manifest_sha1') -> bytes:
    return subprocess.check_output(['py', '-3', '-m', 'build_scripts.digest_sign',
                                    '--key', key, '--get_cert', cert])


def signature_b64(path: str, key: str = 'manifest_sha1', algorithm: str = 'sha1') -> bytes:
    return subprocess.check_output(['py', '-3', '-m', 'build_scripts.digest_sign',
                                    '--key', key, '--algorithm', algorithm, '--file', path, '--base64'])


def pem_signature(path: str) -> bytes:
    signature = signature_b64(path)
    pem_lines = [signature[i:i + 64] for i in range(0, len(signature), 64)]
    return b'\n'.join([b'-----BEGIN SIGNATURE-----', *pem_lines, b'-----END SIGNATURE-----\n'])


def EscapeXML_NL(txt):
    return txt.replace("\n", r"&#x0A;")


def b64_signature(path: str) -> str:
    signature = signature_b64(path).decode()
    pem_lines = [signature[i:i + 64] for i in range(0, len(signature), 64)]
    # Unsure whether inserting these newlines is actually necessary but it's what the existing signing server does.
    return '\n'.join(pem_lines)


def main():
    if not sys.argv[2:]:
        Usage()
    tosign = os.path.abspath(sys.argv[1])
    feedbackname = os.path.abspath(sys.argv[2])
    print(f"Signing warehouse file '{tosign}'")

    signature = b64_signature(tosign)
    pub = get_certificate('pub').decode()
    ca = get_certificate('ca').decode()

    # Wrap elements of signature/certs
    sig_xml = "<signature>%s</signature>" % EscapeXML_NL(signature)
    pub_xml = "<signing_cert>%s</signing_cert>" % EscapeXML_NL(pub)
    ca_xml = "<intermediate_cert>%s</intermediate_cert>" % EscapeXML_NL(ca)

    inner = sig_xml + pub_xml + ca_xml
    outer = '<?xml version="1.0" encoding="utf-8" ?><signatureFile xmlns="badger:signature">%s</signatureFile>' % inner

    md5_hash = hashlib.md5(outer.encode()).hexdigest()
    d = os.path.dirname(tosign)
    fname = md5_hash + "x000.xml"
    outfile = os.path.join(d, fname)
    open(outfile, "wb").write(outer.encode())
    open(feedbackname, "wb").write((md5_hash + "x000").encode())
    print(fname)


if __name__ == "__main__":
    main()
