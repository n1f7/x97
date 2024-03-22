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
            std::copy(std::istream_iterator<std::uint8_t>{ss}, std::istream_iterator<std::uint8_t>{}, std::back_inserter(byteArgs));
            pack.appendArguments(&addr, std::next(&addr));
            pack.appendArguments(std::cbegin(byteArgs), std::cend(byteArgs));

        default:
            std::cin >> std::hex;
            std::copy(std::istream_iterator<std::uint16_t>{ss}, std::istream_iterator<std::uint16_t>{}, std::back_inserter(wordArgs));
            pack.appendArguments(std::begin(wordArgs), std::end(wordArgs));
    }

    return pack;
}

class Exchange : public X97::Packet {
public:
    std::future<const X97::Packet*> transmit(const X97::Packet &pack) {
    }

private:
    inline void _OnTxComplete(const boost::system::error_code &err, std::size_t sz) {
    }

    boost::asio::io_context _Ctx;
    boost::asio::serial_port _COM{_Ctx};
};

int main(int argc, char **argv) try {
    std::ios::sync_with_stdio(false);

    if (argc < 2) {
        std::cerr << "x97 <COM_PORT>\n";
    }

    Exchange proto;

    for (std::string line; std::getline(std::cin, line);) {
        auto pack{makePacket(std::stringstream{std::move(line)})};

        auto response = proto.transmit(pack);

        std::cout << pack << '\n';

        respose.get();
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
} catch (const std::exception &err) {
    std::cerr << "Failure: " << err.what() << '\n';
}
