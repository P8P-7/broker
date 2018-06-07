#include <google/protobuf/util/json_util.h>
#include <boost/log/trivial.hpp>
#include <boost/format.hpp>
#include <zmq.hpp>

#include <MessageCarrier.pb.h>
#include <goliath/foundation.h>

constexpr unsigned PUBLISHING_PORT = 5555;
constexpr unsigned SUBSCRIBING_PORT = 5556;

int main(int argc, char *argv[]) {
    boost::log::trivial::severity_level minSeverity = boost::log::trivial::info;

    bool log = false;
    if (argc >= 2) {
        if (std::strcmp(argv[1], "-l") == 0 || std::strcmp(argv[1], "--log") == 0) {
            minSeverity = boost::log::trivial::trace;
            log = true;
        }
    }

    goliath::util::Console console(&goliath::util::colorConsoleFormatter, argv[0], "logo-broker.txt", minSeverity);
    BOOST_LOG_TRIVIAL(debug) << "Enabled logging mode";

    BOOST_LOG_TRIVIAL(info) << "Starting broker...";

    zmq::context_t context(1);

    BOOST_LOG_TRIVIAL(info) << "Publishing to subscribers on port: " << PUBLISHING_PORT;
    zmq::socket_t xpub(context, ZMQ_XPUB);
    xpub.bind((boost::format("tcp://*:%1%") % PUBLISHING_PORT).str());

    BOOST_LOG_TRIVIAL(info) << "Subscribing to publishers on port: " << SUBSCRIBING_PORT;
    zmq::socket_t xsub(context, ZMQ_XSUB);
    xsub.bind((boost::format("tcp://*:%1%") % SUBSCRIBING_PORT).str());

    if (log) {
        BOOST_LOG_TRIVIAL(debug) << "Starting logging sockets...";
        zmq::socket_t capture_push(context, ZMQ_PUSH);
        capture_push.bind("inproc://capture");

        zmq::socket_t capture_pull(context, ZMQ_PULL);
        capture_pull.connect("inproc://capture");

        google::protobuf::util::JsonPrintOptions options;
        options.add_whitespace = true;
        options.always_print_primitive_fields = true;
        options.preserve_proto_field_names = true;

        std::thread capture_thread([&capture_pull, &options] {
            BOOST_LOG_TRIVIAL(debug) << "Listening on logging pull socket...";
            while (true) {
                zmq::message_t message;
                capture_pull.recv(&message);

                goliath::proto::MessageCarrier carrier;
                carrier.ParseFromArray(message.data(), static_cast<int>(message.size()));

                std::string jsonString;
                google::protobuf::util::MessageToJsonString(carrier, &jsonString, options);

                BOOST_LOG_TRIVIAL(trace) << jsonString;
            }
        });

        BOOST_LOG_TRIVIAL(info) << "Starting proxy...";
        zmq::proxy(xpub, xsub, capture_push);

        zmq::message_t exit_message(4);
        memcpy(exit_message.data(), "exit", 4);

        capture_thread.join();
        capture_pull.send(exit_message);
    } else {
        BOOST_LOG_TRIVIAL(info) << "Starting proxy...";
        zmq::proxy(xpub, xsub, nullptr);
    }

    BOOST_LOG_TRIVIAL(fatal) << "Stopping broker...";
    return 0;
}