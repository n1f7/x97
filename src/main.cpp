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
            std::cin >> std::hex;
            std::copy(std::istream_iterator<std::uint16_t>{ss}, std::istream_iterator<std::uint16_t>{},
                      std::back_inserter(wordArgs));
            pack.appendArguments(std::begin(wordArgs), std::end(wordArgs));
    }

    return pack;
}

class Exchange : public X97::Packet {
public:
    inline std::future<const X97::Packet *> request(const X97::Packet &pack) {
        _Task = std::move(std::packaged_task<const X97::Packet *(X97::Packet *)>{});
        _Tx(pack);
        return _Task.get_future();
    }

private:
    inline void _Tx(const X97::Packet &pack) {
        boost::asio::async_write(_COM,
                                 boost::asio::buffer(static_cast<const X97::Packet *>(&pack), sizeof(X97::Packet)),
                                 std::bind_front(&Exchange::_OnTxComplete, this));
    }

    inline void _Rcv() {}

    inline void _OnTxComplete(const boost::system::error_code &err, std::size_t sz) {
        if (!err && sz == length() && isValid()) {
            _Task(nullptr);
            //            _Task(static_cast<const X97::Packet *>(this));
        }
    }

    std::packaged_task<const X97::Packet *(X97::Packet *)> _Task;
    boost::asio::io_context _Ctx;
    boost::asio::serial_port _COM{_Ctx};
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> _WorkGuard{_Ctx.get_executor()};
    std::jthread _WorkerThread{[this] { _Ctx.run(); }};
};

int main(int argc, char **argv) try {
    std::ios::sync_with_stdio(false);

    if (argc < 2) {
        std::cerr << "x97 <COM_PORT>\n";
    }

    Exchange proto;

    for (std::string line; std::getline(std::cin, line);) {
        auto pack{makePacket(std::stringstream{std::move(line)})};

        auto response = proto.request(pack);

        std::cout << "Sent request\n" << pack << '\n';

        if (auto responsePtr = response.get(); responsePtr) {
            std::cout << "Received\n" << *responsePtr << '\n';
        }
    }
} catch (const std::exception &err) {
    std::cerr << "Failure: " << err.what() << '\n';
}

//    std::vector<std::uint16_t> arguments{0x224, 0x96};
//    X97::Packet proto{X97::Command::SetRegsRpl};
//    proto.appendArguments(std::begin(arguments), std::end(arguments));
//
// #if 0
//    std::cout << std::setfill('0') << std::hex << std::setw(2);
//    std::copy_n(reinterpret_cast<const char *>(&proto), proto.length(), std::ostream_iterator<unsigned
//    short>(std::cout, " ")); std::cout << std::dec << "\nlength:" << proto.length() << '\n';
// #else
//    std::copy_n(reinterpret_cast<const char *>(&proto), proto.length(), std::ostream_iterator<char>(std::cout,
//    ""));
// #endif

