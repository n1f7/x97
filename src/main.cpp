#include <iostream>
#include <iterator>
#include <boost/asio.hpp>
#include <x97.h>
#include <cstring>
#include <vector>
#include <iomanip>

int main(int argc, char **argv) {
    std::vector<std::uint16_t> arguments{0x224, 0x96};
    X97::Packet proto{X97::Packet::Command::SetRegsRpl};
    proto.appendArguments(std::begin(arguments), std::end(arguments));

#if 0
    std::cout << std::setfill('0') << std::hex << std::setw(2);
    std::copy_n(reinterpret_cast<const char *>(&proto), proto.length(), std::ostream_iterator<unsigned short>(std::cout, " "));
    std::cout << std::dec << "\nlength:" << proto.length() << '\n';
#else
    std::copy_n(reinterpret_cast<const char *>(&proto), proto.length(), std::ostream_iterator<char>(std::cout, ""));
#endif
}
