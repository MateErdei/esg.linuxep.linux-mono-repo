#!/usr/bin/env python3
# Copyright 2022-2023 Sophos Limited. All rights reserved.

"""HTTP Server that exposes locally-built SDDS3 repo over HTTP port 8080"""
import traceback
from http.server import HTTPServer, SimpleHTTPRequestHandler
from http import HTTPStatus
import socketserver

import argparse
import base64
import datetime
import email.utils
import hashlib
import os
import json
import subprocess
import ssl
import sys
import tempfile
import time


class SophosThreadingHTTPServer(socketserver.ThreadingMixIn, HTTPServer):
    daemon_threads = True


def hash_file(name):
    sha256 = hashlib.sha256()
    with open(name, 'rb') as f:
        for byte_block in iter(lambda: f.read(65536), b""):
            sha256.update(byte_block)
    return sha256.hexdigest()


def get_ims(when):
    ims = None
    try:
        ims = email.utils.parsedate_to_datetime(when)
    except (TypeError, IndexError, OverflowError, ValueError):
        # ignore ill-formed values
        pass
    else:
        if ims.tzinfo is None:
            # obsolete format with no timezone, cf.
            # https://tools.ietf.org/html/rfc7231#section-7.1.1.1
            ims = ims.replace(tzinfo=datetime.timezone.utc)
    return ims


def get_modtime(stat):
    # compare to UTC datetime of last modification
    last_modif = datetime.datetime.fromtimestamp(
        stat.st_mtime, datetime.timezone.utc)
    # remove microseconds, like in If-Modified-Since
    last_modif = last_modif.replace(microsecond=0)
    return last_modif


def b64u_encode(data):
    encoded = base64.b64encode(data, b'-_')
    encoded = encoded.replace(b'=', b'')
    encoded = encoded.replace(b'\n', b'')
    return encoded


def json_b64u_encode(obj):
    return b64u_encode(json.dumps(obj).encode('utf-8'))


def get_cert(content):
    cert = content

    # remove the begin,end and newlines from the certificate or signature passed in.
    cert = cert.replace('-----BEGIN CERTIFICATE-----', '')
    cert = cert.replace('-----END CERTIFICATE-----', '')
    cert = cert.replace('-----BEGIN SIGNATURE-----', '')
    cert = cert.replace('-----END SIGNATURE-----', '')
    cert = cert.replace('\n', '')
    return cert


def get_signature(data: bytes, private_key: str):
    """
    Use openssl to generate a signature for input data.
    """
    with tempfile.TemporaryDirectory() as tmpdir:
        tmpcertfile = os.path.join(tmpdir, 'cert.pem')
        tmpfile = os.path.join(tmpdir, 'data')
        with open(tmpfile, 'wb') as f:
            f.write(data)
        with open(tmpcertfile, 'w') as f:
            f.write(private_key)
        try:
            result = subprocess.run([
                'openssl',
                'dgst', '-sha384',
                '-sign', tmpcertfile,
                '-keyform', 'pem',
                tmpfile
            ], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,timeout=5)
            if result.returncode != 0:
                raise RuntimeError(
                    f"Signing failed.\nReturn Code:{result.returncode}\nOutput:\n{result.stdout}\n{result.stderr}.")
        except subprocess.CalledProcessError as e:
            raise RuntimeError("command '{}' return with error (code {}): {}".format(e.cmd, e.returncode, e.output))
        return base64.b64encode(result.stdout).decode()


INT_CERT = """
MIIDozCCAougAwIBAgIBNjANBgkqhkiG9w0BAQwFADCBjzEnMCUGA1UEAwweU29w
aG9zIENlcnRpZmljYXRpb24gQXV0aG9yaXR5MSIwIAYJKoZIhvcNAQkBFhNzb3Bo
b3NjYUBzb3Bob3MuY29tMRQwEgYDVQQIDAtPeGZvcmRzaGlyZTELMAkGA1UEBhMC
VUsxDzANBgNVBAoMBlNvcGhvczEMMAoGA1UECwwDRVNHMB4XDTIyMDcxOTAwMDAw
MFoXDTQwMDEwMTAwMDAwMFowgYcxHzAdBgNVBAMMFlNvcGhvcyBCdWlsZCBBdXRo
b3JpdHkxIjAgBgkqhkiG9w0BCQEWE3NvcGhvc2NhQHNvcGhvcy5jb20xFDASBgNV
BAgMC094Zm9yZHNoaXJlMQswCQYDVQQGEwJVSzEPMA0GA1UECgwGU29waG9zMQww
CgYDVQQLDANFU0cwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDb/wjO
JnTnTnVnqOUNeQyvCwlM9EoclTYPITK04TMAS1z8rC0qsUyvBiOh9mwtiNFH5rBe
3sWdwRh5VNZRyQTass2KfbFbxhbCHppjkbYhAt2LlLdBJoXZsUxMWB81Fhe3H2hr
jMNxlXBKYclyCREOoBNfpBFEvb/ixW0DYihChQFzEnONfAU20zBsl16fiNegomcf
Jk6zkcNpt/v181TeudReFQ7gpuSA8VHZuYsYMSntQoCEFQBweb4PqWaf/i+AKT0k
265CE2VBkX4/kyDu1kgXW/quf/OJKZ+Gwh0+vchAkmSbNVOtorxN1uKny/77wU/N
jRQqGm+d++wa3pEDAgMBAAGjEDAOMAwGA1UdEwQFMAMBAf8wDQYJKoZIhvcNAQEM
BQADggEBAA4uedsR4LLAY8keZkNRXPGOEgOObmIyfVKQB3/AgDx+12jK2YaZZd+l
aDLIZLFGcrp+spvxCNagSZB0L9GhG7pPcki5OYqjFe8sGQ8O80N8zW/cxlM9sqwX
Bqvk011Vocu6aOdh0Wj1/iQG8HTI9TgOsHgpD2xZaTiEWOQ8qeLvWl1HIq7TRKQu
B2/shyv+Cw6b28iEKinEhh8ynrUub2r1ddYMpx2IOZNs7ohZJJ1QqeyObee575mp
q6InPHuNGeBL6rRlgIKIER3gvkdkPtH6f7Q7YZvo6YA3uQDc05LGb/y2mrrdZ2rH
5gjCs552y8i7fnSYP84yQgB7UU4lfMI=
"""
SIG_CERT = """
MIIDojCCAoqgAwIBAgIBNzANBgkqhkiG9w0BAQwFADCBhzEfMB0GA1UEAwwWU29w
aG9zIEJ1aWxkIEF1dGhvcml0eTEiMCAGCSqGSIb3DQEJARYTc29waG9zY2FAc29w
aG9zLmNvbTEUMBIGA1UECAwLT3hmb3Jkc2hpcmUxCzAJBgNVBAYTAlVLMQ8wDQYD
VQQKDAZTb3Bob3MxDDAKBgNVBAsMA0VTRzAeFw0yMTAxMDEwMDAwMDFaFw0zNTA5
MTMwMDAwMDBaMIGRMRswGQYDVQQDDBJEYWlseSBCdWlsZCBzaGEzODQxJDAiBgkq
hkiG9w0BCQEWFURhaWx5QnVpbGRAc29waG9zLmNvbTEUMBIGA1UECAwLT3hmb3Jk
c2hpcmUxCzAJBgNVBAYTAlVLMRMwEQYDVQQKDApTb3Bob3MgUGxjMRQwEgYDVQQL
DAtEZXZlbG9wbWVudDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALkf
f8z24cIf2qhkfRc7KDBTXh5abwVLjCGzA3okmpQJy+3ena/Rrt+wChLAQciv+tgc
ry0tymqxhgpMwYezNvigZWNsIurVwBI1aXDB2rYvi10npZLsLHSbUErWtc1bv2xt
cHSbhlFAcDQ2b7P7JCYCVKfoJ72qj3wNakpFxVeCYr58WaSkwe05JqXUBV6C8iwb
S3dHca6vebdXB1cRl3OCT96I7q7hXUcpPCl8IkMdk4cocVKRW4JV1C2OncBNRTbQ
SDp/0xhQOozUWaxafN6cxyEkeQHQ972OJucOMwTrhFo8CDtLrzCCePbsEJy7KWsa
HRVdP2x2l0D/EteGyC0CAwEAAaMNMAswCQYDVR0TBAIwADANBgkqhkiG9w0BAQwF
AAOCAQEAqFianpqzxBvPhnp4nFYJ+9MA7y64Ugaq+47yl2q59BZKwbo/TUOgDvp/
Fwf2MLBDXHQ7PKDJL4qCyokN50ZXnzNQ6IV/TYFXp1If/GcsIW/ay0SA113pqJjJ
hN2JbDoCeT88UKMNFy5KZV/dkkdVIsNwVr3EZwY2DGV6k/y1jMUce3DzJcoJHYZ2
HD9eoMjRMdsOaZPcX4QLAxzRsbln31gA+U5HaZzbzWpx1sKhxT4jNFE2rjRkuLPB
SkMH5+l6g2pW3v/bfROdrE4r6i6AYBsqOIes2yJftyWzCNhPQ/gtGC82aNgacoDv
9Hfe7YzJjXh61dVgD/gZcRkOs0O3Xw==
"""
PRIVATE_KEY = """
-----BEGIN RSA PRIVATE KEY-----
MIIEowIBAAKCAQEAuR9/zPbhwh/aqGR9FzsoMFNeHlpvBUuMIbMDeiSalAnL7d6d
r9Gu37AKEsBByK/62ByvLS3KarGGCkzBh7M2+KBlY2wi6tXAEjVpcMHati+LXSel
kuwsdJtQSta1zVu/bG1wdJuGUUBwNDZvs/skJgJUp+gnvaqPfA1qSkXFV4JivnxZ
pKTB7TkmpdQFXoLyLBtLd0dxrq95t1cHVxGXc4JP3ojuruFdRyk8KXwiQx2Thyhx
UpFbglXULY6dwE1FNtBIOn/TGFA6jNRZrFp83pzHISR5AdD3vY4m5w4zBOuEWjwI
O0uvMIJ49uwQnLspaxodFV0/bHaXQP8S14bILQIDAQABAoIBAQCgmgstriE9YJU5
8bP0K4Y6JplIi/w1A82Wjs89b/QK6lTZEICMv8SOmxKiIdjwWnuscXYoI8mvtkMr
PFvJdlkCaWSMVIwWX0o6gcvv3r33wFePgY3LYMlQgE2wH4Wpmyb+0hL1ltd4Ngch
xPgzlHVP5EW9RVL04LuKS7kiplUGDMYASIdJJHQi0mJGefx4et8elxEfJ+URRjSA
8ytChGUlBLXAxwep7BzBRiwyTm0t6T0tWYtgxR6zrmB2P1s3LnIkhpXZa0V6zZdi
4aI4RPbfvTb+AQQhiFJIrnbDyT2/CN3yK+of7wVmaiw8Tg/HtHf6w2DQYJ6GSFXn
HdHFzJuhAoGBAN4fc+7V3ys8P85N+nFxA0jyYniTqDxXydHS1wcsEl+Y45lCb+vf
kPNWDvpXJ7rMGEpQzOwlP1eyAPwif8ftYeepE6w9SfDjo7pWyiNgcJ+RdCrIeCLX
kibv+drLDAwDUDiEGd6i79S4n2W72/MQm/vRN+ASncFFhXvtf1oLWFXnAoGBANVb
bax3VRuchVXIJMargvDUCxi91NxBMqRwOevTnKqumBvpYHlsD0tMwyuO/raC17Sr
Nv3ojet/AnIVXFhxJsIAmphQoTHSCPTYKj9mt3mliQtP4lJ6u1vEUY2fBCBpyPv2
8KaIuHmVdkfBhLJJkntyEMwRi87OxJoob/DUZsbLAoGANjmZdMHTZFul+/g/XnhH
ZASAE42AcZLA2y9MfRy+M4ZAccatSfjfCviEWYrzUP/IIkRNcoy5RPBYmzTU2vrR
fttgyRiBN4RrEO9lE3PUqq+4m0UrRt43eLf21/nfrAMXD2T4Z8iBIf4cM5rD3De+
zJ/LszD4QBl3t8RH5bSFURsCgYBpkRp8CnOO/OwwXJ5turFITfLLpCntbUkMegb+
u665+TeEH/4/Ngt/O5UaOV+omKb4WvsTuPx3uFlSb2VI0XvW5AuaL9MCXqVV2JtW
0ZEY3KIpebZHDzkjF8kuZK7bBtyOZ0n9bIqyhhSHPqZUvPiAohjTkB74DfDTQgzZ
QY807wKBgB2QmOvHUFUhFA2218I0uAl4YstzxMgRG7+/Nsd7TCu0gxDWLW6xDGgd
bIzgYmUHyt59sOMDghfPS2BklBmcFZwjIMSJ+joOuXxrMpwSCIFkfJYyxqw9R812
4o8o8CKOvH0jgncQYmUClkmoZw7AigjYyeRBgABwZV6dxcseibbw
-----END RSA PRIVATE KEY-----
"""
dictOfRequests = {}


# Private key created using `openssl genrsa -out key.pem 2048`
FAULT_INJECTION_PRIVATE_KEY = """
-----BEGIN RSA PRIVATE KEY-----
MIIEowIBAAKCAQEAuR9/zPbhwh/aqGR9FzsoMFNeHlpvBUuMIbMDeiSalAnL7d6d
r9Gu37AKEsBByK/62ByvLS3KarGGCkzBh7M2+KBlY2wi6tXAEjVpcMHati+LXSel
kuwsdJtQSta1zVu/bG1wdJuGUUBwNDZvs/skJgJUp+gnvaqPfA1qSkXFV4JivnxZ
pKTB7TkmpdQFXoLyLBtLd0dxrq95t1cHVxGXc4JP3ojuruFdRyk8KXwiQx2Thyhx
UpFbglXULY6dwE1FNtBIOn/TGFA6jNRZrFp83pzHISR5AdD3vY4m5w4zBOuEWjwI
O0uvMIJ49uwQnLspaxodFV0/bHaXQP8S14bILQIDAQABAoIBAQCgmgstriE9YJU5
8bP0K4Y6JplIi/w1A82Wjs89b/QK6lTZEICMv8SOmxKiIdjwWnuscXYoI8mvtkMr
PFvJdlkCaWSMVIwWX0o6gcvv3r33wFePgY3LYMlQgE2wH4Wpmyb+0hL1ltd4Ngch
xPgzlHVP5EW9RVL04LuKS7kiplUGDMYASIdJJHQi0mJGefx4et8elxEfJ+URRjSA
8ytChGUlBLXAxwep7BzBRiwyTm0t6T0tWYtgxR6zrmB2P1s3LnIkhpXZa0V6zZdi
4aI4RPbfvTb+AQQhiFJIrnbDyT2/CN3yK+of7wVmaiw8Tg/HtHf6w2DQYJ6GSFXn
HdHFzJuhAoGBAN4fc+7V3ys8P85N+nFxA0jyYniTqDxXydHS1wcsEl+Y45lCb+vf
kPNWDvpXJ7rMGEpQzOwlP1eyAPwif8ftYeepE6w9SfDjo7pWyiNgcJ+RdCrIeCLX
kibv+drLDAwDUDiEGd6i79S4n2W72/MQm/vRN+ASncFFhXvtf1oLWFXnAoGBANVb
bax3VRuchVXIJMargvDUCxi91NxBMqRwOevTnKqumBvpYHlsD0tMwyuO/raC17Sr
Nv3ojet/AnIVXFhxJsIAmphQoTHSCPTYKj9mt3mliQtP4lJ6u1vEUY2fBCBpyPv2
8KaIuHmVdkfBhLJJkntyEMwRi87OxJoob/DUZsbLAoGANjmZdMHTZFul+/g/XnhH
ZASAE42AcZLA2y9MfRy+M4ZAccatSfjfCviEWYrzUP/IIkRNcoy5RPBYmzTU2vrR
fttgyRiBN4RrEO9lE3PUqq+4m0UrRt43eLf21/nfrAMXD2T4Z8iBIf4cM5rD3De+
zJ/LszD4QBl3t8RH5bSFURsCgYBpkRp8CnOO/OwwXJ5turFITfLLpCntbUkMegb+
u665+TeEH/4/Ngt/O5UaOV+omKb4WvsTuPx3uFlSb2VI0XvW5AuaL9MCXqVV2JtW
0ZEY3KIpebZHDzkjF8kuZK7bBtyOZ0n9bIqyhhSHPqZUvPiAohjTkB74DfDTQgzZ
QY807wKBgB2QmOvHUFUhFA2218I0uAl4YstzxMgRG7+/Nsd7TCu0gxDWLW6xDGgd
bIzgYmUHyt59sOMDghfPS2BklBmcFZwjIMSJ+joOuXxrMpwSCIFkfJYyxqw9R812
4o8o8CKOvH0jgncQYmUClkmoZw7AigjYyeRBgABwZV6dxcseibbw
-----END RSA PRIVATE KEY-----
"""


def sign_response(data, certs=[get_cert(SIG_CERT), get_cert(INT_CERT)], private_key=PRIVATE_KEY, sig=None):
    """Generate a JWT-format signature for the content"""
    header = json_b64u_encode({
        'typ': 'JWT',
        'alg': 'RS384',
        'x5c': certs,
    })

    iat = datetime.datetime.utcnow()
    exp = iat + datetime.timedelta(days=7)
    sha256 = hashlib.sha256()
    sha256.update(data)
    payload = json_b64u_encode({
        'size': len(data),
        'sha256': sha256.hexdigest(),
        'iat': int(iat.timestamp()),
        'exp': int(exp.timestamp()),
    })

    sign_data = header + b'.' + payload
    if not sig:
        sig = get_signature(sign_data, private_key)
    return (sign_data + b'.' + b64u_encode(base64.b64decode(sig))).decode('utf-8')


class SDDS3RequestHandler(SimpleHTTPRequestHandler):
    def __init__(self, wwwroot, mode, datadir, *args, **kwargs):
        self.mode = mode
        self.data = datadir
        buffer = 1
        self.log_file = open('/tmp/sdds3_server.log', 'a', buffer)
        super().__init__(*args, directory=wwwroot, **kwargs)

    # Note: must either disable W0622 (redefining builtin 'format'),
    # or disable W0221 (arguments differ from overridden method)
    def log_message(self, format, *args):   # pylint: disable=W0622
        self.log_file.write("%s - - [%s] %s\n" %
                         (self.address_string(),
                          self.log_date_time_string(),
                          format % args))


    def log_date_time_string(self):
        now = time.time()
        msec = int(1000 * (now - int(now)))
        year, month, day, hour, mins, sec, _, _, _ = time.localtime(now)
        return "%02d/%3s/%04d %02d:%02d:%02d.%03d" % (
            day, self.monthname[month], year, hour, mins, sec, msec)

    def _not_modified(self):
        self.send_response(HTTPStatus.NOT_MODIFIED)
        self.end_headers()

    def normal_handle_get_request(self):
        self.log_message("%s", "Headers:\n{}".format(self.headers))
        if self.path.startswith("/v3"):
            self.path = self.path[3:]
        translated_path = self.translate_path(self.path)
        self.log_message("%s", "path: {} -> {}".format(self.path, translated_path))

        # Workaround for the Python server not returning 'Last-Modified' when responding to 'If-Modified-Since'
        # This is needed for SDDS3 supplement caching to work
        if "If-Modified-Since" in self.headers:
            try:
                fs = os.stat(translated_path)

                requested_time = email.utils.parsedate_to_datetime(self.headers["If-Modified-Since"])
                modified_time = datetime.datetime.fromtimestamp(fs.st_mtime, datetime.timezone.utc)
                modified_time = modified_time.replace(microsecond=0)

                if modified_time <= requested_time:
                    self.send_response(HTTPStatus.NOT_MODIFIED)
                    self.send_header("Last-Modified", self.date_time_string(fs.st_mtime))
                    self.end_headers()
                    return None
            except OSError:
                pass  # Will be handled by do_GET below

        try:
            h = hash_file(translated_path)
            self.log_message("SHA256: %s", h)
        except EnvironmentError:
            self.log_message("File path missing %s", translated_path)

        return SimpleHTTPRequestHandler.do_GET(self)

    def retry_5_times_get_request(self):
        if self.path in dictOfRequests.keys():
            dictOfRequests[self.path] += 1
        else:
            dictOfRequests[self.path] = 1

        if dictOfRequests[self.path] == 5:
            self.normal_handle_get_request()
        else:
            self.send_response(202)
            self.send_header("Content-Length", "0")
            self.end_headers()

    def return_202s_then_404(self):
        if self.path in dictOfRequests.keys():
            dictOfRequests[self.path] += 1
        else:
            dictOfRequests[self.path] = 1

        if dictOfRequests[self.path] == 5:
            self.send_response(404)
            self.send_header("Content-Length", "0")
            self.end_headers()
        else:
            self.send_response(202)
            self.send_header("Content-Length", "0")
            self.end_headers()

    def cdn_hang(self):
        secs_to_hang = 720
        self.log_message(f'CDN GET request will now hang for {secs_to_hang/60} mins...')
        time.sleep(secs_to_hang)
        self.log_message('CDN GET request finished and will not send a response')

    def return_exitCode(self, exitCode):
        self.send_response(exitCode)
        self.send_header("Content-Length", "0")
        self.end_headers()

    def do_GET(self):
        if os.environ.get("COMMAND") == "retry":
            self.retry_5_times_get_request()
        elif os.environ.get("COMMAND") == "failure":
            self.return_202s_then_404()
        elif os.environ.get("COMMAND") == "cdn_hang":
            self.cdn_hang()
        elif os.environ.get("EXITCODE") is not None:
            self.return_exitCode(int(os.environ.get("EXITCODE")))
        else:
            self.normal_handle_get_request()

    def do_POST(self):          # pylint: disable=C0103
        """Read the SUS request from the client, and log it
        Return either mock_sus_response_winep.json or mock_sus_response_winsrv.json"""

        try:
            if os.environ.get("COMMAND") == "sus_hang":
                self.sus_hang()
            elif os.environ.get("COMMAND") == "sus_invalid_json":
                self.sus_invalid_json()
            elif os.environ.get("COMMAND") == "sus_large_json":
                self.sus_large_json()
            elif os.environ.get("COMMAND") == "sus_missing_body":
                self.sus_missing_body()
            elif os.environ.get("COMMAND") == "sus_no_certs":
                self.sus_no_certs()
            elif os.environ.get("COMMAND") == "sus_unverifier_signer":
                self.sus_unverifier_signer()
            elif os.environ.get("COMMAND") == "sus_corrupt_signature":
                self.sus_corrupt_signature()
            elif os.environ.get("COMMAND") == "sus_401":
                self.sus_http_code(401)
            elif os.environ.get("COMMAND") == "sus_404":
                self.sus_http_code(404)
            elif os.environ.get("COMMAND") == "sus_500":
                self.sus_http_code(500)
            elif os.environ.get("COMMAND") == "sus_503":
                self.sus_http_code(503)
            else:
                request_body = self.rfile.read(int(self.headers['Content-Length']))
                self.log_message('Received SUS request: %s', request_body)

                doc = json.loads(request_body)
                if 'product' not in doc:
                    self.send_error(HTTPStatus.BAD_REQUEST, "Product not specified :-(")
                    return

                if self.mode == 'launchdarkly':
                    self.log_message('launchdarkly')
                    self.respond_launchdarkly(doc)
                    return

                self.log_message('respond_mock_sus')
                self.respond_mock_sus(doc)
        except Exception as ex:
            self.log_message(traceback.format_exc())

    def respond_mock_sus(self, doc):
        product = doc['product']
        mock_response = f'{self.data}/mock_sus_response_{product}_RECOMMENDED.json'
        self.log_message('sus file '+ mock_response)
        try:
            with open(mock_response, 'rb') as f:
                stat = os.fstat(f.fileno())
                data = f.read()
        except OSError:
            self.send_error(HTTPStatus.NOT_FOUND, 'File not found')
            return

        self.log_message('Signing Mock SUS response...')

        try:
            sig = sign_response(data)
        except Exception as ex:
            self.log_message(traceback.format_exc())
            self.log_message('Calculating signature of SUS failed with error: %s', ex)
            raise  # sig is used below, so better to re-raise original exception

        self.log_message('SUS response: %s', data)

        self.send_response(HTTPStatus.OK)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', str(stat[6]))
        self.send_header('Last-Modified', self.date_time_string(stat.st_mtime))
        self.send_header('X-Content-Signature', sig)
        self.end_headers()
        self.wfile.write(data)

    def respond_launchdarkly(self, doc):
        product = doc['product']

        suites = []
        error = ""
        reason = ""
        code = ""

        if 'platform_token' not in doc:
            self.send_error(HTTPStatus.BAD_REQUEST, 'Missing platform_token')
            return
        else:
            platform = doc['platform_token']
            if not platform:
                self.send_error(HTTPStatus.BAD_REQUEST, 'Empty platform_token')

            self.log_message(f"Package requested for {platform}")

        if 'subscriptions' not in doc:
            self.send_error(HTTPStatus.BAD_REQUEST, 'Missing subscriptions')
            return

        if 'fixed_version_token' not in doc:
            self.log_message('No fixed_version_token for ESM')

            for subscription in doc['subscriptions']:

                if 'id' not in subscription or 'tag' not in subscription:
                    self.send_error(HTTPStatus.BAD_REQUEST, 'Missing subscriptions')
                    return

                line_id = subscription['id']
                tag = subscription['tag']
                self.log_message('line_id '+ line_id)
                self.log_message('tag '+ tag)
                fixedVersion = subscription.get('fixedVersion', False)
                if fixedVersion:
                    self.log_message(f"{line_id} fixedVersion: {fixedVersion} requested")
                flag = f'{self.data}/release.{product}.{line_id}.json'
                self.log_message('flag data file ' + flag)
                if not os.path.exists(flag):
                    continue

                with open(flag, 'r') as f:
                    flagdata = json.load(f)

                if tag in flagdata:
                    self.log_message(flagdata[tag]['suite'])
                    suites.append(flagdata[tag]['suite'])
                else:
                    # We found the subscription, but the tag isn't found
                    # Return an error??
                    # ==> use RECOMMENDED for now
                    suites.append(flagdata['RECOMMENDED']['suite'])
        else:
            fixed_version_token = doc['fixed_version_token']
            self.log_message(f'fixed_version_token {fixed_version_token}')
            filepath = f'{self.data}/linuxep.json'
            if os.path.exists(filepath):

                with open(filepath, 'r') as f:
                    suiteinfo = json.load(f)

                if fixed_version_token != suiteinfo['token']:
                    self.log_message(f'Incorrect token {fixed_version_token} provided ' + suiteinfo['token'])

                jsonsuites = suiteinfo['suite_info']
                for suite in jsonsuites:
                    suites.append(jsonsuites[suite]['suite'])
            else:
                self.log_message('could not find linuxep.json file returning error message SUS')
                error = "Did not return any suites"
                reason = "Fixed version token not found"
                code = "FIXED_VERSION_TOKEN_NOT_FOUND"

        if not code:
            data = json.dumps({
                'suites': suites,
                'release-groups': ['0', 'GranularInitial'],
            }).encode('utf-8')
        else:
            data = json.dumps({
                'suites': suites,
                'release-groups': ['0', 'GranularInitial'],
                'error': error,
                'code': code,
                'reason': reason
            }).encode('utf-8')

        self.log_message('Signing LaunchDarkly SUS response...')
        try:
            sig = sign_response(data)
        except Exception as ex:
            self.log_message(traceback.format_exc())
            self.log_message('Calculating signature of SUS failed with error: %s', ex)
            raise  # sig is used below, so better to re-raise original exception
        self.log_message('SUS response: %s', data)
        self.log_message('SUS sign: %s', sig)

        self.send_response(HTTPStatus.OK)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', str(len(data)))
        self.send_header('Last-Modified', self.date_time_string())
        self.send_header('X-Content-Signature', sig)
        self.end_headers()
        self.wfile.write(data)

    def sus_hang(self):
        secs_to_hang = 60
        self.log_message(f'SUS POST request will now hang for {secs_to_hang/60} mins...')
        time.sleep(secs_to_hang)
        self.log_message('SUS POST request finished and will not send a response')

    def sus_invalid_json(self):
        response = "this is not { json }".encode('utf-8')
        self.log_message(f"Responding to SUS POST request with invalid JSON payload")
        self.send_response(HTTPStatus.OK)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', str(len(response)))
        self.send_header("X-Content-Signature", sign_response(response))
        self.end_headers()
        self.wfile.write(response)

    def sus_large_json(self):
        arbitrary_data_array = ["some data"] * 10000

        response = json.dumps({
            'suites': arbitrary_data_array,
            'release-groups': ['0'],
        }).encode('utf-8')
        self.log_message(f"Responding to SUS POST request with large JSON payload: size={len(response)}")
        self.send_response(HTTPStatus.OK)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', str(len(response)))
        self.send_header("X-Content-Signature", sign_response(response))
        self.end_headers()
        self.wfile.write(response)

    def sus_missing_body(self):
        response = json.dumps({
            "suites": ["suite"],
            "release-groups": ["0"],
        }).encode("utf-8")
        self.log_message(f"Responding to SUS POST request with missing body payload")
        self.send_response(HTTPStatus.OK)
        self.send_header("Content-Type", "application/json")
        self.send_header('Content-Length', str(len(response)))
        self.send_header("X-Content-Signature", sign_response(response))
        self.end_headers()
        # no data written

    def sus_no_certs(self):
        response = json.dumps({
            "suites": ["suite"],
            "release-groups": ["0"],
        }).encode("utf-8")
        self.log_message(f"Responding to SUS POST request with payload without certs")
        self.send_response(HTTPStatus.OK)
        self.send_header("Content-Type", "application/json")
        self.send_header('Content-Length', str(len(response)))
        self.send_header("X-Content-Signature", sign_response(response, certs=[]))
        self.end_headers()
        self.wfile.write(response)

    def sus_unverifier_signer(self):
        response = json.dumps({
            "suites": ["suite"],
            "release-groups": ["0"],
        }).encode("utf-8")
        self.log_message(f"Responding to SUS POST request with unverified signature")
        self.send_response(HTTPStatus.OK)
        self.send_header("Content-Type", "application/json")
        self.send_header('Content-Length', str(len(response)))
        self.send_header("X-Content-Signature", sign_response(response, private_key=FAULT_INJECTION_PRIVATE_KEY))
        self.end_headers()
        self.wfile.write(response)

    def sus_corrupt_signature(self):
        response = json.dumps({
            "suites": ["suite"],
            "release-groups": ["0"],
        }).encode("utf-8")
        self.log_message(f"Responding to SUS POST request with corrupt signature")
        self.send_response(HTTPStatus.OK)
        self.send_header("Content-Type", "application/json")
        self.send_header('Content-Length', str(len(response)))
        self.send_header("X-Content-Signature", sign_response(response, sig="foo="))
        self.end_headers()
        self.wfile.write(response)

    def sus_http_code(self, http_code):
        self.log_message(f"Responding to SUS POST request with {http_code}")
        self.send_response(http_code)
        self.send_header("Content-Length", "0")
        self.end_headers()


def make_request_handler(args):     # noqa: C901
    wwwroot = args.wwwroot

    if args.launchdarkly:
        print(f'Starting server in LaunchDarkly mode from {args.launchdarkly}')
        mode = 'launchdarkly'
        datadir = args.launchdarkly
    elif args.mock_sus:
        print(f'Starting server in Mock SUS mode from {args.mock_sus}')
        mode = 'mock_sus'
        datadir = args.mock_sus
    else:
        raise NameError('Cannot start server: neither LaunchDarkly nor Mock SUS mode selected')

    class DynamicSDDS3RequestHandler(SDDS3RequestHandler):
        def __init__(self, *args, **kwargs):
            super().__init__(wwwroot, mode, datadir, *args, **kwargs)

    return DynamicSDDS3RequestHandler


def main():
    print("Starting SDDS3server.py:", sys.argv)
    parser = argparse.ArgumentParser()
    parser.add_argument('--launchdarkly',
                        help='Path to launchdarkly flags (used to calculate suites as per SUS)')
    parser.add_argument('--mock-sus',
                        help='Path to folder containing "mock_sus_response_<product>_*.json" files')
    parser.add_argument('--sdds3', dest='wwwroot',
                        help='Path to (local) SDDS3 repository to serve')
    parser.add_argument('--protocol',
                        help='tsl setting option : tls1_1')
    parser.add_argument('--port',
                        help='port number of server default 8080',
                        default=8080,
                        type=int)
    parser.add_argument('--certpath', help='path to certificate, default is common/TA/utils/server_certs/server.crt')
    args = parser.parse_args()

    protocol = ssl.PROTOCOL_TLS
    if args.protocol:
        if args.protocol == "tls1_1":
            protocol = ssl.PROTOCOL_TLSv1_1

    if args.certpath:
        cert = args.certpath
    else:
        import PathManager
        cert = os.path.join(PathManager.get_utils_path(), "server_certs", "server.crt")
    assert os.path.isfile(cert)
    port = int(args.port)
    if args.launchdarkly and args.mock_sus:
        raise NameError('error: --launchdarkly and --mock-sus are mutually exclusive. Pick one or the other')

    if not (args.launchdarkly or args.mock_sus):
        if os.path.exists('output/launchdarkly'):
            args.launchdarkly = os.path.abspath('output/launchdarkly')
        elif os.path.exists('output/sus/mock_sus_response_winep_RECOMMENDED.json'):
            args.mock_sus = os.path.abspath('output/sus')
        else:
            raise NameError('error: could not find output/launchdarkly or '
                            + 'output/sus/mock_sus_response_*: specify --launchdarkly or --mock-sus')

    HandlerClass = make_request_handler(args)
    httpd = SophosThreadingHTTPServer(('0.0.0.0', port), HandlerClass)
    httpd.socket = ssl.wrap_socket(httpd.socket,
                                   server_side=True,
                                   certfile=cert,
                                   ssl_version=protocol)
    httpd.allow_reuse_address = True
    print('Listening on 0.0.0.0 port %d...' % port)
    httpd.serve_forever()


if __name__ == '__main__':
    main()
