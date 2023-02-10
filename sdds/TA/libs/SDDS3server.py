#!/usr/bin/env python3

"""HTTP Server that exposes locally-built SDDS3 repo over HTTP port 8080"""
from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
from http import HTTPStatus

import argparse
import base64
import datetime
import email.utils
import hashlib
import os
import json
import subprocess
import sys
import tempfile
import time


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


def get_cert(name):
    result = subprocess.run([
        'digest_sign',
        '--key', 'manifest_sha384',
        '--get_cert', name,
    ], check=True, capture_output=True)

    cert = result.stdout.decode('utf-8')

    # remove the begin,end and newlines from the certificate or signature passed in.
    cert = cert.replace('-----BEGIN CERTIFICATE-----', '')
    cert = cert.replace('-----END CERTIFICATE-----', '')
    cert = cert.replace('-----BEGIN SIGNATURE-----', '')
    cert = cert.replace('-----END SIGNATURE-----', '')
    cert = cert.replace('\n', '')
    return cert


def get_signature(data):
    with tempfile.TemporaryDirectory() as tmpdir:
        # cannot use tempfile.NamedTemporaryFile(), because we must close it
        # but it gets deleted on close.
        #
        # TemporaryDirectory() deletes the whole directory on close.
        tmpfile = os.path.join(tmpdir, 'data')
        with open(tmpfile, 'wb') as f:
            f.write(data)

        try:
            result = subprocess.run([
                'digest_sign',
                '--key', 'manifest_sha384',
                '--algorithm', 'sha384',
                '--file', tmpfile,
                '--base64',
            ], check=True, capture_output=True)
            return result.stdout
        except subprocess.CalledProcessError as e:
            print(f'Failed: {e}: {e.stderr}')
            raise


SIG_CERT = get_cert('pub')
INT_CERT = get_cert('ca')


def sign_response(data):
    """Generate a JWT-format signature for the content"""
    header = json_b64u_encode({
        'typ': 'JWT',
        'alg': 'RS384',
        'x5c': [SIG_CERT, INT_CERT],
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
    sig = get_signature(sign_data)
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

    def do_POST(self):          # pylint: disable=C0103
        """Read the SUS request from the client, and log it
        Return either mock_sus_response_winep.json or mock_sus_response_winsrv.json"""

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
        sig = sign_response(data)
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

        if 'subscriptions' not in doc:
            self.send_error(HTTPStatus.BAD_REQUEST, 'Missing subscriptions')
            return

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

        data = json.dumps({
            'suites': suites,
            'release-groups': ['0','GranularInitial'],
        }).encode('utf-8')

        self.log_message('Signing LaunchDarkly SUS response...')
        sig = sign_response(data)
        self.log_message('SUS response: %s', data)

        self.send_response(HTTPStatus.OK)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', len(data))
        self.send_header('Last-Modified', self.date_time_string(time.time()))
        self.send_header('X-Content-Signature', sig)
        self.end_headers()
        self.wfile.write(data)


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
    parser = argparse.ArgumentParser()
    parser.add_argument('--launchdarkly',
                        help='Path to launchdarkly flags (used to calculate suites as per SUS)')
    parser.add_argument('--mock-sus',
                        help='Path to folder containing "mock_sus_response_<product>_*.json" files')
    parser.add_argument('--sdds3', dest='wwwroot',
                        help='Path to (local) SDDS3 repository to serve')
    args = parser.parse_args()

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
    httpd = ThreadingHTTPServer(('0.0.0.0', 8080), HandlerClass)
    httpd.allow_reuse_address = True
    print('Listening on 0.0.0.0 port 8080...')
    httpd.serve_forever()


if __name__ == '__main__':
    main()
