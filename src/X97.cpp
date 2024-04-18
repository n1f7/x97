#include <cctype>
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
#include <X97.h>

#include "MM3.h"

// Hate this fucking cruft (https://en.cppreference.com/w/cpp/string/byte/tolower)
static inline char safeToLower(char ch) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
}

static inline X97::Packet makePacket(std::stringstream ss) {
    std::string cmdStr;
    ss >> cmdStr;
    std::transform(std::begin(cmdStr), std::end(cmdStr), std::begin(cmdStr), ::safeToLower);
    X97::Packet pack(X97::fromString(cmdStr));

    std::uint16_t addr = 0;
    std::vector<std::uint8_t> byteArgs;
    std::vector<std::uint16_t> wordArgs;

    ss >> std::hex;
    switch (pack.command()) {
        case X97::Command::Write:
        case X97::Command::WriteRpl: // 16bit address followed by bytes
            ss >> addr;
            // Stupid fucking `generic` C++ treats std::uint8_t same as signed char, resulting in this crap
            std::transform(std::istream_iterator<std::uint16_t>(ss), std::istream_iterator<std::uint16_t>(),
                           std::back_inserter(byteArgs),
                           [](const std::uint16_t &x) { return 0x7f & static_cast<uint8_t>(x); });
            pack.appendArguments(&addr, std::next(&addr));
            pack.appendArguments(std::cbegin(byteArgs), std::cend(byteArgs));
            break;

        default: // Usually 16bit address value pairs
            std::copy(std::istream_iterator<std::uint16_t>(ss), std::istream_iterator<std::uint16_t>(),
                      std::back_inserter(wordArgs));
            pack.appendArguments(std::begin(wordArgs), std::end(wordArgs));
    }

    return pack;
}

int main(int argc, char **argv) try {
    std::ios::sync_with_stdio(false);

    if (argc < 2) {
        std::vector<std::string> ports;
        Xprom::MM3::enumerate(std::back_inserter(ports));

        std::cout << "Usage:  X97 <COM_PORT>\n\n";
        if (ports.empty()) {
            std::cout << "No serial ports available\n";
        } else {
            std::cout << "Choose one of " << ports.size() << " serial ports available:\n";
            std::copy(std::begin(ports), std::end(ports), std::ostream_iterator<std::string>(std::cout, "\n"));
        }

        return 1;
    }

    Xprom::MM3 com{argv[1]};

    std::cout << "X97 Protocol console v1.0\nIn order to send command type: <CMD> <Arg1> <Arg2> ... <ArgN> and press "
                 "enter\nFor example: SetRegsRpl 224 96\nTo repeat last sent command type r and press enter\nTo quit "
                 "type: q and press enter\n\n>";
    std::string previousLine = "SetRegsRpl 224 96";
    for (std::string line; std::getline(std::cin, line);) {
        if (line == "q") {
            break;
        } else if (line == "r") {
            line = previousLine;
            std::cout << "\n>" << line << '\n';
        } else {
            previousLine = line;
        }

        try {
            auto pack{makePacket(std::stringstream{std::move(line)})};
            auto response = com.request(pack);
            std::cout << pack.length() << " bytes out\n" << pack << '\n';

            if (auto responsePtr = response.get(); responsePtr) {
                std::cout << '\n' << responsePtr->length() << " bytes in\n" << *responsePtr << '\n';
                if (!responsePtr->isValid()) {
                    std::cout << "BAD PACKET CHECKSUM!\n";
                }
            }
        } catch (const boost::system::system_error &err) {
            if (err.code() == boost::asio::error::operation_aborted) {
                std::cout << "\nIO operation timed out\n";
            } else {
                std::cout << "\nSerial communication error: " << err.what() << 'n';
            }
        } catch (const X97::ProtocolError &err) {
            std::cout << "\nProtocol error: " << err.what() << '\n';
        } catch (const std::out_of_range &) {
            std::cout << "\nUnknown command\n";
        }

        std::cout << "\n\n>";
    }
} catch (const std::exception &err) {
    std::cerr << "Failure: " << err.what() << '\n';
}

