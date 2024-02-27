#ifndef DICOMCONNECTION_H
#define DICOMCONNECTION_H

#include <boost/optional.hpp>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/log/trivial.hpp>

#include <functional>
#include <memory>
#include <chrono>

using namespace boost;

/**
 * @brief The tcpconnection class encapsulates functionality to
 * communicate over a TCP socket using boost asio.
 */
class tcpconnection : public std::enable_shared_from_this<tcpconnection>
{
public:
    tcpconnection(asio::io_context& ctx) :
        context_(ctx),
        socket_(ctx)
    {}

    // tcpconnection(asio::io_context& ctx,
    //               std::shared_ptr<boost::asio::ip::tcp::socket> sock,
    //               std::function<void(tcpconnection*)> on_end_connection);

    // void write_data(std::shared_ptr<std::vector<unsigned char>> buffer,
    //                 std::function<void(const boost::system::error_code&, std::size_t)> on_complete) override;

    // void write_data(void* data_offset, std::size_t len,
    //                 std::function<void (const boost::system::error_code&, std::size_t)> on_complete) override;

    // void read_data(std::shared_ptr<std::vector<unsigned char>> buffer, std::size_t len,
    //                std::function<void(const boost::system::error_code&, std::size_t)> on_complete) override;

    // void read_data(std::shared_ptr<std::vector<unsigned char>> buffer,
    //                std::function<void(const boost::system::error_code&, std::size_t)> on_complete) override;

    // std::unique_ptr<Iinfrastructure_timeout_connection> timeout_timer(
    //     std::chrono::duration<int> timeout,
    //     std::function<void()> on_timeout) override;

    // bool is_stopped() const override
    // {
    //     return ctx_.stopped();
    // }

    void close();

    asio::awaitable<void> run(auto /*sft*/)
    {
        auto len = buffer_.size();

        //co_await asio::async_read(socket_, asio::buffer(buffer_), asio::transfer_exactly(len), asio::use_awaitable);

        auto rsize = co_await asio::async_read(socket_, asio::buffer(buffer_), asio::use_awaitable);

        co_return;
    }

    asio::awaitable<void> wathdog() {
        auto executor = co_await asio::this_coro::executor;
        for(;;) {

        }
        co_return;
    }

    void start(std::shared_ptr<tcpconnection> sft)
    {
        co_spawn(context_, run(sft->shared_from_this()), asio::detached);
    }

    asio::ip::tcp::socket& socket() { return socket_; }

protected:
    asio::ip::tcp::socket socket_;

private:
    asio::io_context& context_;
    std::shared_ptr<boost::asio::ip::tcp::socket> skt;
    std::vector<unsigned char> buffer_;

    // data::dataset::commandset_processor proc;
    // std::deque<std::unique_ptr<property>> send_queue;
    // boost::optional<std::vector<unsigned char>*> received_pdu;
    // std::map<TYPE, std::function<void(Connection*, std::unique_ptr<property>)>> handlers;

    bool shutdown_requested;

    //std::function<void(tcpconnection*)> handler_end_connection;
};

#endif // DICOMCONNECTION_H
