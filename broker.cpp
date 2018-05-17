#include <memory>
#include <iostream>
#include <ostream>
#include <thread>
#include <chrono>
#include <iomanip>

#include <boost/log/core.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/format.hpp>
#include <zmq.hpp>

constexpr unsigned PUBLISHING_PORT = 5555;
constexpr unsigned SUBSCRIBING_PORT = 5556;

void coloringFormatter(boost::log::record_view const& rec, boost::log::formatting_ostream& strm) {
    auto now = std::chrono::high_resolution_clock::now();
    auto now_c = std::chrono::high_resolution_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%F %T");

    strm << '[' << ss.str() << "] ";

    auto severity = rec.attribute_values()["Severity"].extract<boost::log::trivial::severity_level>();
    if (severity) {
        strm << '(' << severity << ") ";
        switch (severity.get()) {
            case boost::log::trivial::debug:
                strm << "\033[32m";
                break;
            case boost::log::trivial::info:
                strm << "\033[34m";
                break;
            case boost::log::trivial::fatal:
                strm << "\033[31m";
                break;
            default:
                break;
        }
    }

    strm << rec[boost::log::expressions::smessage];

    // Reset color
    if (severity) {
        strm << "\033[0m";
    }
}

int main(int argc, char* argv[]) {
    bool log = false;
    if(argc >= 3) {
        if(argv[1] == "-l" || argv[1] == "--log") {
            if(argv[2] == "true") {
                log = true;
            }
        }
    }

    auto sink = boost::make_shared<boost::log::sinks::synchronous_sink<boost::log::sinks::text_ostream_backend>>();
    sink->locked_backend()->add_stream(boost::shared_ptr<std::ostream>(&std::cout, boost::null_deleter()));

    sink->set_formatter(&coloringFormatter);

    boost::log::core::get()->add_sink(sink);

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