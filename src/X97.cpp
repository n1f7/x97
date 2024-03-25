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

#include "SerialPort.h"

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

int main(int argc, char **argv) try {
    std::ios::sync_with_stdio(false);

    if (argc < 2) {
        std::cerr << "x97 <COM_PORT>\n";
        return 1;
    }

    Xprom::SerialPort com{argv[1]};

    std::cout << "In order to send command type: <CMD> <Arg1> <Arg2> ... <ArgN> and press enter\nFor example: "
                 "SetRegsRpl 224 96\nTo repeat last sent command type r and press enter\nTo quit type: q and press "
                 "enter\n\n>";
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

        auto pack{makePacket(std::stringstream{std::move(line)})};
        auto response = com.request(pack);
        std::cout << pack.length() << " bytes out\n" << pack << '\n';

        if (auto responsePtr = response.get(); responsePtr) {
            std::cout << '\n' << responsePtr->length() << " bytes in\n" << *responsePtr << '\n';
        }

        std::cout << "\n\n>";
    }
} catch (const std::exception &err) {
    std::cerr << "Failure: " << err.what() << '\n';
}

