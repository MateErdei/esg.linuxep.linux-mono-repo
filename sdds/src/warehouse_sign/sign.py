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


def pem_signature(path: str, key: str = 'manifest_sha1', algorithm: str = 'sha1') -> bytes:
    signature = signature_b64(path, key, algorithm)
    pem_lines = [signature[i:i + 64] for i in range(0, len(signature), 64)]
    return b'\n'.join([b'-----BEGIN SIGNATURE-----', *pem_lines, b'-----END SIGNATURE-----\n'])


def EscapeXML_NL(txt):
    return txt.replace("\n", r"&#x0A;")


def b64_signature(path: str, key: str = 'manifest_sha1', algorithm: str = 'sha1') -> str:
    signature = signature_b64(path, key, algorithm).decode()
    pem_lines = [signature[i:i + 64] for i in range(0, len(signature), 64)]
    # Unsure whether inserting these newlines is actually necessary but it's what the existing signing server does.
    return '\n'.join(pem_lines)


def main():
    if not sys.argv[2:]:
        Usage()
    tosign = os.path.abspath(sys.argv[1])
    feedbackname = os.path.abspath(sys.argv[2])
    print(f"Signing warehouse file '{tosign}'")

    # sha1 certs
    signature = b64_signature(tosign)
    pub = get_certificate('pub').decode()
    ca = get_certificate('ca').decode()

    # sha384 certs
    signature_384 = b64_signature(tosign, key="manifest_sha384")
    pub_384 = get_certificate('pub', key="manifest_sha384").decode()
    ca_384 = get_certificate('ca', key="manifest_sha384").decode()

    # Wrap elements of signature/certs
    # sha1 certs
    sig_xml = "<signature>%s</signature>" % EscapeXML_NL(signature)
    pub_xml = "<signing_cert>%s</signing_cert>" % EscapeXML_NL(pub)
    ca_xml = "<intermediate_cert>%s</intermediate_cert>" % EscapeXML_NL(ca)
    inner = sig_xml + pub_xml + ca_xml

    # sha384 certs
    sig_384_xml = "<xsig>%s</xsig>" % EscapeXML_NL(signature_384)
    pub_384_xml = "<xsig_signing_cert>%s</xsig_signing_cert>" % EscapeXML_NL(pub_384)
    ca_384_xml = "<xsig_intermediate_cert>%s</xsig_intermediate_cert>" % EscapeXML_NL(ca_384)
    inner_384 = sig_384_xml + pub_384_xml + ca_384_xml

    outer = '<?xml version="1.0" encoding="utf-8" ?>'
    outer += '<signatureFile xmlns="badger:signature">%s</signatureFile>' % inner
    outer += '<extSignature algorithm="sha384">%s</extSignature>' % inner_384

    md5_hash = hashlib.md5(outer.encode()).hexdigest()
    d = os.path.dirname(tosign)
    fname = md5_hash + "x000.xml"
    outfile = os.path.join(d, fname)
    open(outfile, "wb").write(outer.encode())
    open(feedbackname, "wb").write((md5_hash + "x000").encode())
    print(fname)


if __name__ == "__main__":
    main()
