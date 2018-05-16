#include <iostream>
#include <zmq.hpp>

int main(int argc, char *argv[]) {
    std::cout << "Starting broker..." << std::endl;

    zmq::context_t context(1);

    //  Socket facing subscribers
    std::cout << "Publishing on port: 5555" << std::endl;
    zmq::socket_t xpub(context, ZMQ_XPUB);
    xpub.bind("tcp://*:5555");

    //  Socket facing publishers
    std::cout << "Subscribing on port: 5556" << std::endl;
    zmq::socket_t xsub(context, ZMQ_XSUB);
    xsub.bind("tcp://*:5556");

    //  Start the proxy
    std::cout << "Starting proxy..." << std::endl;
    zmq::proxy(xpub, xsub, nullptr);

    std::cout << "Stopping broker..." << std::endl;
    return 0;
}