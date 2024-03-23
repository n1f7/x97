#pragma once

#include <array>
#include <string>
#include <climits>
#include <iomanip>
#include <utility>
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <numeric>
#include <iterator>
#include <iostream>
#include <type_traits>
#include <unordered_map>

namespace X97 {
    class Packet;
}

std::ostream &operator<<(std::ostream &, const X97::Packet &);

namespace X97 {
    enum class Command : std::uint8_t {
        GetRegs = 1,
        SetRegs,
        SetRegsRpl,
        SetRegsBits,
        SetRegsBitsRpl,
        Exec,
        ExecRpl,
        Read,
        Write,
        WriteRpl,
    };

    namespace _Impl {
        template <class T, std::size_t... N>
        T septet(const T &x, std::index_sequence<N...>) {
            return ((((0x7f << (7 * N)) & x) << N) | ...);
        }

        inline const std::unordered_map<std::string, Command> cmdLUT{{"GetRegs", Command::GetRegs}};

    } // namespace _Impl

    template <class T>
    T septet(const T &x) {
        return _Impl::septet(x, std::make_index_sequence<sizeof(T)>{});
    }

    inline Command fromString(const std::string &x) {
        return _Impl::cmdLUT.at(x);
    }

    class Packet {
        friend std::ostream & ::operator<<(std::ostream &, const Packet &);

    public:
        Packet() = default;

        Packet(Packet &&) noexcept            = default;
        Packet &operator=(Packet &&) noexcept = default;

        explicit Packet(Command cmd)
            : _Header{.magic = 0x95, .address = std::byte(3), .command = cmd, .length = sizeof(_Header)} {
            _Header.crc = _headerCRC();
        }

        template <class InputIter>
        InputIter appendArguments(InputIter first, InputIter last) {
            static_assert(std::is_same_v<typename std::iterator_traits<InputIter>::value_type, std::uint16_t> ||
                          std::is_same_v<typename std::iterator_traits<InputIter>::value_type, std::uint8_t>);

            auto i = 0;
            for (; i < (_Data.size() - 2) && first != last;
                 ++first, i += sizeof(typename std::iterator_traits<InputIter>::value_type)) {
                const auto v = septet(*first);
                std::copy_n(reinterpret_cast<const std::uint8_t *>(&v), sizeof(v), &_Data[i]);
            }
            _Header.length = sizeof(_Header) + i + sizeof(std::uint16_t);
            _Header.crc    = _headerCRC();
            if (5 < length()) {
                const auto crc = septet(_packetCRC());
                std::copy_n(reinterpret_cast<const std::uint8_t *>(&crc), sizeof(crc), &_Data[i]);
            }
            return first;
        }

        // TODO:
        template <class OutputIter>
        OutputIter extractArguments(OutputIter d_first) {
            return d_first;
        }

        std::ostream &serialize(std::ostream &str) const {
            return str.write(reinterpret_cast<const char *>(this), length());
        }

        std::istream &deserialize(std::istream &str) {
            return str.read(reinterpret_cast<char *>(&_Header), sizeof(_Header)) && isValid()
                       ? str.read(reinterpret_cast<char *>(_Data.data()), length())
                       : str;
        }

        std::size_t length() const {
            return static_cast<std::size_t>(_Header.length);
        }

        Command command() const {
            return _Header.command;
        }

        bool isValid() const {
            return _Header.magic == 0x95 && sizeof(_Header) <= _Header.length && _Header.crc == _headerCRC();
        }

    private:
        std::byte _headerCRC() const {
            auto p  = reinterpret_cast<const std::uint8_t *>(&_Header);
            int sum = std::accumulate(p, p + 4, int(0));
            return std::byte(((sum >> 7) + sum) & 0x7f);
        }

        static inline const std::uint16_t _CRC16LUT[] = {
            0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241, 0xC601, 0x06C0, 0x0780, 0xC741, 0x0500,
            0xC5C1, 0xC481, 0x0440, 0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40, 0x0A00, 0xCAC1,
            0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841, 0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81,
            0x1A40, 0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41, 0x1400, 0xD4C1, 0xD581, 0x1540,
            0xD701, 0x17C0, 0x1680, 0xD641, 0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040, 0xF001,
            0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240, 0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0,
            0x3480, 0xF441, 0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41, 0xFA01, 0x3AC0, 0x3B80,
            0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840, 0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
            0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40, 0xE401, 0x24C0, 0x2580, 0xE541, 0x2700,
            0xE7C1, 0xE681, 0x2640, 0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041, 0xA001, 0x60C0,
            0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240, 0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480,
            0xA441, 0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41, 0xAA01, 0x6AC0, 0x6B80, 0xAB41,
            0x6900, 0xA9C1, 0xA881, 0x6840, 0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41, 0xBE01,
            0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40, 0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1,
            0xB681, 0x7640, 0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041, 0x5000, 0x90C1, 0x9181,
            0x5140, 0x9301, 0x53C0, 0x5280, 0x9241, 0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
            0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40, 0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901,
            0x59C0, 0x5880, 0x9841, 0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40, 0x4E00, 0x8EC1,
            0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41, 0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680,
            0x8641, 0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040};

        static std::uint16_t crc16(const std::uint8_t *data, std::size_t sz) {
            std::uint16_t reg_crc = 0xFFFF;
            while (sz--) {
                const std::uint8_t i = *data++ ^ reg_crc;
                reg_crc >>= 8;
                reg_crc ^= _CRC16LUT[i];
            }
            return reg_crc;
        }

        std::uint16_t _packetCRC() const {
            const std::uint16_t crc = crc16(reinterpret_cast<const std::uint8_t *>(this), length() - 2);
            return ((crc >> 14) + crc) & 0x3fff;
        }

        struct {
            std::uint8_t magic;
            std::byte address;
            Command command;
            char length;
            std::byte crc;
        } _Header                           = {};
        std::array<std::uint8_t, 127> _Data = {};

        static_assert(sizeof(_Header) == 5, "Wrong header size");
    };

    class WritePacket : public Packet {
    public:
        template <class InputIter>
        WritePacket(std::uint16_t address, InputIter first, InputIter last) {}
    };
} // namespace X97

std::ostream &operator<<(std::ostream &str, const X97::Packet &x) {
    assert(x.isValid());
    str << std::hex << std::setw(2) << std::setfill('0') << int(x._Header.magic) << ' ' << std::hex << std::setw(2)
        << std::setfill('0') << int(x._Header.address) << ' ' << std::hex << std::setw(2) << std::setfill('0')
        << int(x._Header.command) << ' ' << std::hex << std::setw(2) << std::setfill('0') << int(x._Header.length)
        << ' ' << std::hex << std::setw(2) << std::setfill('0') << int(x._Header.crc);

    for (auto i = 0; i < (x.length() - 5 - 2); ++i) {
        if (i % 8 == 0) {
            str << '\n';
        }
        str << std::hex << std::setw(2) << std::setfill('0') << int(x._Data[i]) << ' ';
    }

    if (5 < x.length()) {
        str << '\n'
            << std::hex << std::setw(2) << std::setfill('0')
            << int((std::uint16_t(x._Data[x.length() - 6]) << CHAR_BIT) | std::uint16_t(x._Data[x.length() - 7]));
    }

    return str;
}
