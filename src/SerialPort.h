using namespace std::chrono_literals;

#if defined(_WIN32)
#    include <windows.h>
#endif

namespace Xprom {
    class SerialPort {
        enum {
            BaudRate = 115200,
        };

    public:
        SerialPort(const char *port) : _COM{_IoContext, port} {
            _COM.set_option(boost::asio::serial_port::baud_rate(BaudRate));
            _COM.set_option(boost::asio::serial_port::parity(boost::asio::serial_port::parity::odd));
            _COM.set_option(boost::asio::serial_port::stop_bits(boost::asio::serial_port::stop_bits::one));
        }

        inline std::future<X97::Packet *> request(const X97::Packet &pack) {
            _Promise = std::promise<X97::Packet *>{};
            _DoTx(pack);
            return _Promise.get_future();
        }

        template <class OutputIter>
        static OutputIter enumerate(OutputIter d_first) {
#if defined(_WIN32)
            HKEY key;
            DWORD max_value_len, max_data_size;
            if ((::RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DEVICEMAP\\SERIALCOMM"), 0, KEY_QUERY_VALUE,
                                &key) == ERROR_SUCCESS) &&
                (::RegQueryInfoKey(key, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &max_value_len, &max_data_size, NULL,
                                   NULL)) == ERROR_SUCCESS) {
                auto value_len = max_value_len + 1;
                auto data_size = max_data_size + 1;
                std::string value((max_value_len + 1) * sizeof(TCHAR), '\0');
                std::string name((max_data_size + 1) * sizeof(TCHAR), '\0');
                DWORD type = 0;
                for (DWORD i = 0; value_len = max_value_len + 1, data_size = max_data_size,
                           ::RegEnumValue(key, i, value.data(), &value_len, NULL, &type, (LPBYTE)name.data(),
                                          &data_size) == ERROR_SUCCESS;
                     ++i) {
                    if (type == REG_SZ) {
                        name.resize(data_size / sizeof(TCHAR));
                        name.shrink_to_fit();
#    if NOT_USED
                        value.resize(value_len / sizeof(TCHAR));
                        value.shrink_to_fit();
#    endif

                        *d_first++ = name;

#    if NOT_USED
                        value.resize((max_value_len + 1) * sizeof(TCHAR));
                        std::fill(value.begin(), value.end(), '\0');
#    endif
                        name.resize((max_data_size + 1) * sizeof(TCHAR));
                        std::fill(name.begin(), name.end(), '\0');
                    }
                }

                ::RegCloseKey(key);
            }
#endif
            return d_first;
        }

    private:
        inline void _DoTx(const X97::Packet &pack) {
            const auto sp = pack.packet();
            boost::asio::async_write(_COM, boost::asio::buffer(sp.data(), sp.size_bytes()),
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
                        return;
                }
            }
            _Promise.set_value(nullptr);
        }

        inline void _DoRx() {
            const auto sp = _Response.header();
            boost::asio::async_read(_COM, boost::asio::buffer(sp.data(), sp.size_bytes()),
                                    std::bind_front(&SerialPort::_OnRxHeaderComplete, this));

            _Timeout.expires_from_now(2s);
            _Timeout.async_wait([this](const boost::system::error_code &err) {
                if (!err) {
                    std::cout << "\nOperation timed out\n";
                    _COM.cancel();
                }
            });
        }

        inline void _OnRxHeaderComplete(const boost::system::error_code &err, std::size_t sz) {
            if (!err && sz == X97::Packet::HeaderSize && _Response.isHeaderValid()) {
                _DoRxBody();
            } else {
                _Promise.set_value(nullptr);
            }
        }

        inline void _DoRxBody() {
            const auto sp = _Response.data();
            boost::asio::async_read(_COM, boost::asio::buffer(sp.data(), sp.size_bytes()),
                                    std::bind_front(&SerialPort::_OnRxComplete, this));
        }

        inline void _OnRxComplete(const boost::system::error_code &err, std::size_t sz) {
            _Timeout.cancel();
            if (!err && sz == (_Response.length() - X97::Packet::HeaderSize)) {
                _Promise.set_value(&_Response);
            } else {
                _Promise.set_value(nullptr);
            }
        }

        X97::Packet _Response;
        std::promise<X97::Packet *> _Promise;

        boost::asio::io_context _IoContext;
        boost::asio::steady_timer _Timeout{_IoContext};
        boost::asio::serial_port _COM;
        std::jthread _WorkerThread{[this] { _IoContext.run(); }};
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> _WorkGuard{_IoContext.get_executor()};
    };

} // namespace Xprom

