"""HTTP Server that exposes locally-built SDDS3 repo over HTTP port 8080"""
from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
from http import HTTPStatus

import argparse
import io
import os
import json


# pylint: disable=R0915  # too many statements: yes, well I'm dynamically building a class
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

    class SDDS3RequestHandler(SimpleHTTPRequestHandler):
        def __init__(self, *args, **kwargs):
            self.mode = mode
            self.data = datadir
            super().__init__(*args, directory=wwwroot, **kwargs)

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
                self.respond_launchdarkly(doc)
                return

            self.respond_mock_sus(doc)

        def respond_mock_sus(self, doc):
            product = doc['product']
            mock_response = f'{self.data}/mock_sus_response_{product}.json'
            try:
                f = open(mock_response, 'rb')
            except OSError:
                self.send_error(HTTPStatus.NOT_FOUND, 'File not found')
                return

            try:
                stat = os.fstat(f.fileno())

                self.send_response(HTTPStatus.OK)
                self.send_header('Content-Type', 'application/json')
                self.send_header('Content-Length', str(stat[6]))
                self.send_header('Last-Modified', self.date_time_string(stat.st_mtime))
                self.end_headers()

                self.copyfile(f, self.wfile)
            except:
                f.close()
                raise

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

                flag = f'{self.data}/release.{product}.{line_id}.json'
                if not os.path.exists(flag):
                    continue

                with open(flag, 'r') as f:
                    flagdata = json.load(f)
                if tag in flagdata:
                    suites.append(flagdata[tag]['suite'])
                else:
                    # We found the subscription, but the tag isn't found
                    # Return an error??
                    # ==> use RECOMMENDED for now
                    suites.append(flagdata['RECOMMENDED']['suite'])

            self.send_response(HTTPStatus.OK)
            self.end_headers()

            with io.StringIO() as f:
                json.dump({
                    'suites': suites,
                    'release-groups': [],
                }, f)
                self.log_message('Sending SUS response: %s', f.getvalue())
                self.wfile.write(f.getvalue().encode('utf-8'))

    return SDDS3RequestHandler


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--launchdarkly',
                        help='Path to launchdarkly flags (used to calculate suites as per SUS)')
    parser.add_argument('--mock-sus',
                        help='Path to folder containing "mock_sus_response_<product>.json" files')
    parser.add_argument('--sdds3', dest='wwwroot',
                        help='Path to (local) SDDS3 repository to serve')
    args = parser.parse_args()

    if args.launchdarkly and args.mock_sus:
        raise NameError('error: --launchdarkly and --mock-sus are mutually exclusive. Pick one or the other')

    if not (args.launchdarkly or args.mock_sus):
        if os.path.exists('output/sus/mock_sus_response_winep.json'):
            args.mock_sus = os.path.abspath('output/sus')
        elif os.path.exists('output/launchdarkly'):
            args.launchdarkly = os.path.abspath('output/launchdarkly')
        else:
            raise NameError('error: could not find output/launchdarkly or '
                            + 'output/sus/mock_sus_response_*: specify --launchdarkly or --mock-sus')

    HandlerClass = make_request_handler(args)
    httpd = ThreadingHTTPServer(('0.0.0.0', 8080), HandlerClass)
    print('Listening on 0.0.0.0 port 8080...')
    httpd.serve_forever()


if __name__ == '__main__':
    main()
