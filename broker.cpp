#include <boost/log/trivial.hpp>
#include <boost/format.hpp>
#include <zmq.hpp>

#include <goliath/foundation.h>

constexpr unsigned PUBLISHING_PORT = 5555;
constexpr unsigned SUBSCRIBING_PORT = 5556;

int main(int argc, char* argv[]) {
    goliath::util::Console console(&goliath::util::colorConsoleFormatter, "logo-broker.txt");

    bool log = false;
    if(argc >= 3) {
        if(argv[1] == "-l" || argv[1] == "--log") {
            if(argv[2] == "true") {
                log = true;
            }
        }
    }

    BOOST_LOG_TRIVIAL(debug) << "Starting broker...";

    zmq::context_t context(1);

    BOOST_LOG_TRIVIAL(debug) << "Publishing to subscribers on port: " << PUBLISHING_PORT;
    zmq::socket_t xpub(context, ZMQ_XPUB);
    xpub.bind((boost::format("tcp://*:%1%") % PUBLISHING_PORT).str());

    BOOST_LOG_TRIVIAL(debug) << "Subscribing to publishers on port: " << SUBSCRIBING_PORT;
    zmq::socket_t xsub(context, ZMQ_XSUB);
    xsub.bind((boost::format("tcp://*:%1%") % SUBSCRIBING_PORT).str());

    if(log) {
        BOOST_LOG_TRIVIAL(debug) << "Starting capture sockets...";
        zmq::socket_t capture_push(context, ZMQ_PUSH);
        capture_push.bind("inproc://capture");

        zmq::socket_t capture_pull(context, ZMQ_PULL);
        capture_pull.connect("inproc://capture");

        std::thread capture_thread([&]{
            BOOST_LOG_TRIVIAL(debug) << "Listening on capture socket...";
            while(true) {
                zmq::message_t message;
                capture_pull.recv(&message);
                BOOST_LOG_TRIVIAL(info) << static_cast<char*>(message.data());
            }
        });

        BOOST_LOG_TRIVIAL(debug) << "Starting proxy...";
        zmq::proxy(xpub, xsub, capture_push);

        zmq::message_t exit_message(4);
        memcpy(exit_message.data(), "exit", 4);

        capture_thread.join();
        capture_pull.send(exit_message);
    } else {
        BOOST_LOG_TRIVIAL(debug) << "Starting proxy...";
        zmq::proxy(xpub, xsub, nullptr);
    }

    BOOST_LOG_TRIVIAL(fatal) << "Stopping broker...";
    return 0;
}