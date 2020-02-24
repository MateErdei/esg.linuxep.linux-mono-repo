#!/usr/bin/env python3

#requires: python3 -m pip install aiohttp_sse
import asyncio
from aiohttp import web
from aiohttp.web import Response
from aiohttp_sse import sse_response
import PathManager
import ssl
import logging
import os

LOGGER = logging.getLogger(__name__)

"""
Implement a Push Server to mimic the requirement around the Push Service: 
https://wiki.sophos.net/pages/viewpage.action?spaceKey=SophosCloud&title=EMP%3A+command+push
"""


class ServerState:
    """Keep the queue of messages for each subscribed sse client"""
    def __init__(self):
        self._queue = asyncio.Queue()

    async def message(self):
        try:
            data = await asyncio.wait_for(self._queue.get(), timeout=10)
            return data
        except asyncio.TimeoutError:
            return None

    async def put(self, message):
        await self._queue.put(message)


class ServerConfig:
    """Configurations to be applied for the Push Server.
    Currently, they are:
    auth: Define the Authorization Header that must be presented to be a valid request
    ping_time: Control the Keep alive interval message that Server send to clients.
    """
    def __init__(self):
        self.auth = None
        self.ping_time = 1


async def subscribe_endpoint(request):
    """
    This is the handler for the Server Sent Events Client.
    A client that issue a get request to the url:
    https://localhost:8459/mcs/push/endpoint/thisendpoint

    Will receive messages from the server whenever they are available.

    For example:
    curl 'https://localhost:8459/mcs/push/endpoint/me'  --cacert SupportFiles/CloudAutomation/root-ca.crt.pem

    See ::send_this to check how to send a message to the client above.

    This handler may reject the request if configured to verify Authorization ( see : set_auth)

    If the connection is accepted, the server will either:
      - Send message to subscribers as instructed ( see ::send_this)
      - Send a keep alive message ( called ping )
      - Close the connection with the client if instructed to do so by a special message ('stop')
        curl -X POST -d'stop' https://localhost:8459/mcs/push/sendmessage -H "Content-Type: application/text" --cacert SupportFiles/CloudAutomation/root-ca.crt.pem

    Messages will escape newlines to prevent them to be 'broken and splited' as different messages.
    Note: This has been checked to be similar to the behaviour of sending messages with new lines via curl.
    For example: Using the 'test service in place socket-server-us-east-1.poc.hydra.sophos.com'
    Terminal1: curl https://su-fa66bce2974e.socket-server-us-east-1.poc.hydra.sophos.com/sub/myspecialid
    Terminal2: curl -X POST "https://publisher.socket-server-us-east-1.poc.hydra.sophos.com/pub/myspecialid" -H "Content-Type: application/json"  -d "{
'this':'message',
'has':'many lines'
}"

    :param request: default requests.request type given by the python aiohttp.web framework.
    :return:
    """
    require_auth = request.app['config'].auth
    ping_time = request.app['config'].ping_time
    authorization = request.headers.get('Authorization', '')
    all_headers = dict(request.headers)
    LOGGER.info("EndpointId Connected with headers: {}.".format(all_headers))
    LOGGER.info("EndpointId Connected with Authorization: {}.".format(authorization))
    if require_auth and require_auth != authorization:
        LOGGER.warning("Rejecting connection due to authentication. Expected {}, received: {}".format(require_auth, authorization))
        raise web.HTTPUnauthorized(reason="Authentication header provided but different from expected")
    if authorization.strip() == "":
        LOGGER.warning("Missing Authorization header. By default it must be provided")
        raise web.HTTPUnauthorized(reason="No Authentication header provided")

    async with sse_response(request) as resp:
        app = request.app
        server_state = ServerState()
        app['channels'].add(server_state)
        resp.ping_interval = ping_time
        endpoint_id = request.match_info.get('endpoint_id', 'no-endpoint-defined')
        LOGGER.info("EndpointId Connected {}".format(endpoint_id))
        try:
            while not resp.task.done():
                data = await server_state.message()
                if not data:
                    LOGGER.debug('No message to send to subscribers. Loop again')
                    continue
                if data == 'stop':
                    LOGGER.info("Request to stop sending messages to subscribers. Closing connections")
                    resp.stop_streaming()
                    await resp.wait()
                    resp.force_close()
                    break
                LOGGER.info("Sending to Subscribers data: {}".format(data))
                # this is necessary to ensure message is not broken
                # verify this with the real server
                data2 = data.replace('\n', '\\n')
                await resp.send(data2)
        finally:
            app['channels'].remove(server_state)
            LOGGER.info("Connection closed with client")
    return resp

async def index(request):
    """Helper handler that allows to verify the Mock Server via browser.
    For example, visit https://127.0.0.1:8459/ in the browser.
    See also how to send crafted messages send_this
    It will serve an html page with a java script that will connect to the Server Side Channel (Push Server)
    and it will update the page whenever a new message is sent from the server.
    """
    d = """
        <html>
        <body>
            <script>
                var evtSource = new EventSource("/mcs/push/endpoint/testendid");
                evtSource.onmessage = function(e) {
                    document.getElementById('response').innerText = e.data
                }
                evtSource.onerror = function(e) {
                    console.log("my object: %o", e)
                    document.getElementById('response').innerText = 'closed'
                }
                
            </script>
            <h1>Response from server:</h1>
            <div id="response"></div>
        </body>
    </html>
    """
    return Response(text=d, content_type='text/html')


async def send_this(request):
    """Handler that allows sending message to subscribers
    Example:
    curl -X POST -d'example message' https://localhost:8459/mcs/push/sendmessage -H "Content-Type: application/text" --cacert SupportFiles/CloudAutomation/root-ca.crt.pem
    """
    text = await request.text()
    LOGGER.info('Request to send subscribers this message: {}'.format(text))
    for server_state in request.app['channels']:
        await server_state.put(text)
    return Response()


async def set_auth(request):
    """Configure the Push Server to validate the Authorization Header and reject if not set to the same value
    Example:
    curl -X PUT -d'Basic xxxocnso' 'https://localhost:8459/mcs/push/authorization' -H "Content-Type: application/text" --cacert SupportFiles/CloudAutomation/root-ca.crt.pem
    """
    text = await request.text()
    request.app['config'].auth = text
    LOGGER.info("Configuring Required Authorization to {}".format(text))
    return Response()


async def set_ping_interval(request):
    """Update the Ping Interval for the Push Server to send the Keep alive message.
    Example:
    curl -X PUT -d'10' 'https://localhost:8459/mcs/push/ping_interval' -H "Content-Type: application/text" --cacert SupportFiles/CloudAutomation/root-ca.crt.pem
    """
    text = await request.text()
    request.app['config'].ping_time = int(text)
    LOGGER.info("Configuring Ping Interval to {}".format(text))
    return Response()


def get_app(certfile, ping_time):
    """Basic configuration of a aiohttp.web.Application to serve the MCS Push Fake server."""
    app = web.Application()
    app['channels'] = set()
    app['config'] = ServerConfig()
    app['config'].ping_time = ping_time
    app.router.add_route('GET', '/mcs/push/endpoint/{endpoint_id}', subscribe_endpoint)
    app.router.add_route('GET', '/', index)
    app.router.add_route('POST', '/mcs/push/sendmessage', send_this)
    app.router.add_route('PUT', '/mcs/push/authorization', set_auth)
    app.router.add_route('PUT', '/mcs/push/ping_interval', set_ping_interval)
    if certfile:
        LOGGER.info("Launching https server")
        ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        ssl_context.load_cert_chain(certfile)
    else:
        LOGGER.info("Launching http server")
        ssl_context = None
    return app, ssl_context



if __name__ == "__main__":
    description="""
    Control the configuration of Mock MCS Push Server
    
    Examples of how to run: 
    python3 libs/MockMCSPushServer.py
    
    MockMCSPushServer uses the certs generated in SupportFiles/CloudAutomation by default. If when launching it produces: 
    ssl_context.load_cert... 
    
    Move to SupportFiles/CloudAutomation 
    make all
    This will generate all the certs. Try to launch the server again. 
    
    This server can be 'tested' with the browser as well. By default the url is: 
    https://127.0.0.1:8459/
    
    The browser will warn about the cert, but after accepting it, there will be a page with the title: 
     Response from server
     
    In order to send a message to the server, run the following curl command: 
    curl -X POST -d'example message' https://localhost:8459/mcs/push/sendmessage -H "Content-Type: application/text" --cacert SupportFiles/CloudAutomation/root-ca.crt.pem
    And the browser should display the new 'example message'. 

    This Server is used by PushServerUtils and has some tests available in TestMCSPushServer.
    For more information on other 'handlers' check those files.         
    
    """
    import argparse
    default_cert = os.path.join( PathManager.get_support_file_path(), 'CloudAutomation/server-private.pem')
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument('--certfile', type=str, help='Path to the private.pem certificate for launching https server',
                        default=default_cert)
    parser.add_argument('--logpath', type=str, help='Path to the log file to be created', default='mcspushserver.log')
    parser.add_argument('--ping-time', type=int, help='Interval for the keep alive ping message [seconds]', default=8)
    parser.add_argument('--port', type=int, help='Port number to listen to.', default=8459)

    args = parser.parse_args()

    #extract parameters
    certfile = args.certfile
    logpath = args.logpath
    ping_time = args.ping_time
    port = args.port

    logging.basicConfig(filename=logpath, level=logging.DEBUG)

    LOGGER.info("Logging information to file: {}".format(os.path.abspath(logpath)))
    LOGGER.info("Applying the certificate: {}".format(os.path.abspath(certfile)))
    LOGGER.info("Setting ping time to: {}".format(ping_time))
    LOGGER.info("Listening on port: {}".format(port))

    app, ssl_context = get_app(certfile=certfile, ping_time=ping_time)
    web.run_app(app, host='127.0.0.1', port=port, ssl_context=ssl_context)
