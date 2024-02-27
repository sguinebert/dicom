#ifndef CONNECTION_H
#define CONNECTION_H

#include <boost/optional.hpp>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/log/trivial.hpp>
#include <deque>

#include <functional>
#include <memory>
#include <chrono>

#include "upperlayer_properties.hpp"

using namespace boost;

#include <boost/asio/experimental/as_tuple.hpp>
constexpr auto use_nothrow_awaitable = asio::experimental::as_tuple(asio::use_awaitable);
/**
 * The dicom namespace is the global namespace of the project, containing all
 * DICOM related implementations.
 */
namespace dicom
{

/**
 * The network namespace contains all DICOM network-related functionality, like
 * the implementation of the upperlayer or the dimse protocol machine.
 */
namespace network
{

/**
 * The upperlayer namespace contains classes and functions which implement the upperlayer as
 * described in the DICOM standard chapter 8.
 * @link http://medical.nema.org/Dicom/2011/11_08pu.pdf
 */
namespace upperlayer
{

class connection : public std::enable_shared_from_this<connection>
{
public:
    connection(asio::io_context& ctx) :
           deadline_(ctx),
           context_(ctx),
           socket_(ctx)
       {}
    asio::awaitable<void> run(auto /*sft*/)
    {
        auto len = buffer_.size();

        //co_await asio::async_read(socket_, asio::buffer(buffer_), asio::transfer_exactly(len), asio::use_awaitable);

        auto rsize = co_await asio::async_read(socket_, asio::buffer(buffer_), asio::use_awaitable);

        co_return;
    }

    void close() {
        shutdown_ = true;
        context_.post([this]() {
            socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
            socket_.close();
        });
    }

    void stop_artim() {

    }
    void start_artim() {

    }

    asio::awaitable<void> wathdog() {
        auto executor = co_await asio::this_coro::executor;
        asio::steady_timer timer(executor);
        for(;;) {

        }
        co_return;
    }

    void start(std::shared_ptr<connection> sft)
    {
        co_spawn(context_, run(sft->shared_from_this()), asio::detached);
    }



    asio::ip::tcp::socket& socket() { return socket_; }

protected:
    asio::steady_timer deadline_;
    std::deque<std::unique_ptr<property>> queue_;
    std::vector<unsigned char> buffer_;

    bool ignorenext_ = false;
    bool shutdown_ = false;
    bool isSCU_ = false;

private:
    asio::io_context& context_;
    asio::ip::tcp::socket socket_;

    friend class StateMachine;
};

} //upperlayer
} //network
} //dicom

#endif // CONNECTION_H
