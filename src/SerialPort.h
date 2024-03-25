using namespace std::chrono_literals;

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
                        return;
                }
            }
            _Promise.set_value(nullptr);
        }

        inline void _DoRx() {
            boost::asio::async_read(_COM, boost::asio::buffer(&_Response, X97::Packet::HeaderSize),
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
            if (!err && sz == X97::Packet::HeaderSize && _Response.isValid()) {
                _DoRxBody();
            } else {
                _Promise.set_value(nullptr);
            }
        }

        inline void _DoRxBody() {
            boost::asio::async_read(_COM,
                                    boost::asio::buffer(_Response.data(), _Response.length() - X97::Packet::HeaderSize),
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

