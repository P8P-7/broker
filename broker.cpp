#include <zmq.hpp>

int main(int argc, char *argv[]) {
    zmq::context_t context(1);

    //  Socket facing subscribers
    zmq::socket_t xpub(context, ZMQ_XPUB);
    xpub.bind("tcp://*:5555");

    //  Socket facing publishers
    zmq::socket_t xsub(context, ZMQ_XSUB);
    xsub.bind("tcp://*:5556");

    //  Start the proxy
    zmq::proxy(xpub, xsub, nullptr);
    return 0;
}