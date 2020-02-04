import zmq
import os


# Globals
ZMQ_CONTEXT = zmq.Context.instance()

def get_context():
    global ZMQ_CONTEXT
    if ZMQ_CONTEXT is None:
        ZMQ_CONTEXT = zmq.Context.instance()
    return ZMQ_CONTEXT

def set_socket_times(socket):
    socket.RCVTIMEO = 5000
    socket.SNDTIMEO = 5000
    return socket


def get_socket(context, address, zmq_type):
    if zmq_type == zmq.REP:
        socket = context.socket(zmq.REP)
        socket.bind(address)
    elif zmq_type == zmq.REQ:
        socket = context.socket(zmq.REQ)
        socket.connect(address)
    elif zmq_type == zmq.PULL:
        socket = context.socket(zmq.PULL)
        socket.bind(address)
    elif zmq_type == zmq.PUSH:
        socket = context.socket(zmq.PUSH)
        socket.connect(address)
    # Note below two are used for the
    # pub sub data channel and as such both
    # connect to the socket as the router
    # already binds the socket at both ends
    elif zmq_type == zmq.PUB:
        socket = context.socket(zmq.PUB)
        socket.connect(address)
    elif zmq_type == zmq.SUB:
        socket = context.socket(zmq.SUB)
        socket.connect(address)
    else:
        raise ValueError("Type not supported")
    return set_socket_times(socket)


def try_get_socket(context, path, zmq_type):
    trimmed_path = path.replace("ipc://", "")
    try:
        return get_socket(context, path, zmq_type)
    except zmq.error.ZMQError:
        try:
            os.system("touch {}".format(trimmed_path))
            return get_socket(context, path, zmq_type)
        except OSError:
            raise AssertionError("Please ensure that the following exists and is accessible: {}".format(trimmed_path))
