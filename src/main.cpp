#include <thread>
#include <future>
#include <sstream>
#include <exception>
#include <iterator>
#include <cstdio>
#include <iostream>
#include <iterator>
#include <cstring>
#include <vector>
#include <iomanip>
#include <string>
#include <functional>

#include <boost/asio.hpp>
#include <x97.h>

inline X97::Packet makePacket(std::stringstream ss) {
    std::string cmdStr;
    ss >> cmdStr;
    X97::Packet pack(X97::fromString(cmdStr));

    std::vector<std::uint16_t> wordArgs;

    std::uint16_t addr{};
    std::vector<std::uint8_t> byteArgs;

    switch (pack.command()) {
        case X97::Command::Read:
        case X97::Command::Write:
        case X97::Command::WriteRpl:
            ss >> std::hex >> addr;
            std::copy(std::istream_iterator<std::uint8_t>{ss}, std::istream_iterator<std::uint8_t>{},
                      std::back_inserter(byteArgs));
            pack.appendArguments(&addr, std::next(&addr));
            pack.appendArguments(std::cbegin(byteArgs), std::cend(byteArgs));

        default:
            ss >> std::hex;
            std::copy(std::istream_iterator<std::uint16_t>{ss}, std::istream_iterator<std::uint16_t>{},
                      std::back_inserter(wordArgs));
            pack.appendArguments(std::begin(wordArgs), std::end(wordArgs));
    }

    return pack;
}

class SerialPort {
public:
    SerialPort(const char *port) : _COM{_IoContext, port} {
        _COM.set_option(boost::asio::serial_port::baud_rate(115200));
        _COM.set_option(boost::asio::serial_port::parity(boost::asio::serial_port::parity::odd));
        _COM.set_option(boost::asio::serial_port::stop_bits(boost::asio::serial_port::stop_bits::one));
    }

    inline std::future<const X97::Packet *> request(const X97::Packet &pack) {
        _Task = std::move(std::packaged_task<const X97::Packet *(X97::Packet *)>{[](X97::Packet *p) { return p; }});
        _DoTx(pack);
        return _Task.get_future();
    }

private:
    inline void _DoTx(const X97::Packet &pack) {
        boost::asio::async_write(_COM, boost::asio::buffer(&pack, pack.length()),
                                 std::bind_front(&SerialPort::_OnTxComplete, this, std::cref(pack)));
    }

    inline void _OnTxComplete(const X97::Packet &pack, const boost::system::error_code &err, std::size_t sz) {
        if (!err && sz == pack.length()) {
            switch (pack.command()) {
                case X97::Command::GetRegs:
                case X97::Command::SetRegsRpl:
                case X97::Command::SetRegsBitsRpl:
                case X97::Command::ExecRpl:
                case X97::Command::Read:
                case X97::Command::WriteRpl:
                    _DoRx();
            }
        } else {
            //           std::cerr << "TX failed: " << err.what() << '\n';
            _Task(nullptr);
        }
    }

    inline void _DoRx() {
        boost::asio::async_read(_COM, boost::asio::buffer(&_Response, X97::Packet::HeaderSize),
                                std::bind_front(&SerialPort::_OnRxHeaderComplete, this));
    }

    inline void _OnRxHeaderComplete(const boost::system::error_code &err, std::size_t sz) {
        if (!err && sz == X97::Packet::HeaderSize && _Response.isValid()) {
            _DoRxBody();
        } else {
            _Task(nullptr);
        }
    }

    inline void _DoRxBody() {
        boost::asio::async_read(_COM,
                                boost::asio::buffer(_Response.data(), _Response.length() - X97::Packet::HeaderSize),
                                std::bind_front(&SerialPort::_OnRxComplete, this));
    }

    inline void _OnRxComplete(const boost::system::error_code &err, std::size_t sz) {
        if (!err && sz == (_Response.length() - X97::Packet::HeaderSize)) {
            _Task(&_Response);
        } else {
            _Task(nullptr);
        }
    }

    X97::Packet _Response;
    std::packaged_task<const X97::Packet *(X97::Packet *)> _Task;
    boost::asio::io_context _IoContext;
    boost::asio::serial_port _COM;
    std::jthread _WorkerThread{[this] { _IoContext.run(); }};
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> _WorkGuard{_IoContext.get_executor()};
};

int main(int argc, char **argv) try {
    std::ios::sync_with_stdio(false);

    if (argc < 2) {
        std::cerr << "x97 <COM_PORT>\n";
        return 1;
    }

    SerialPort com{argv[1]};

    for (std::string line; std::getline(std::cin, line);) {
        auto pack{makePacket(std::stringstream{std::move(line)})};

        auto response = com.request(pack);

        std::cout << "Sent request\n" << pack << '\n';

        if (auto responsePtr = response.get(); responsePtr) {
            std::cout << "Received\n" << *responsePtr << '\n';
        }

        std::cout << '\n';
    }
} catch (const std::exception &err) {
    std::cerr << "Failure: " << err.what() << '\n';
}

