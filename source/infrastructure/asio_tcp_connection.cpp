#include "asio_tcp_connection.hpp"



Iinfrastructure_timeout_connection::~Iinfrastructure_timeout_connection()
{
}

timeout_connection::timeout_connection(boost::asio::io_service& io_svc,
                                       std::chrono::duration<int> timeout,
                                       std::function<void()> on_timeout):
   on_timeout {on_timeout},
   timer {std::make_shared<boost::asio::steady_timer>(io_svc, std::chrono::steady_clock::now() + timeout)}
{
}

void timeout_connection::cancel()
{
   timer->cancel();
}

void timeout_connection::wait_async()
{
   auto handler = [this](const boost::system::error_code& error)
   {
      if (error != boost::asio::error::operation_aborted) {
         on_timeout();
      }
   };

   timer->async_wait(handler);
}


Iinfrastructure_upperlayer_connection::~Iinfrastructure_upperlayer_connection()
{

}

tcpconnection::tcpconnection(asio::io_context& ctx,
                                         std::shared_ptr<boost::asio::ip::tcp::socket> sock,
                             std::function<void(tcpconnection*)> on_end_connection):
    context_ {ctx},
    socket_(ctx),
    skt {sock},
   handler_end_connection {on_end_connection}
{

}

void tcpconnection::write_data(std::shared_ptr<std::vector<unsigned char>> buffer,
                                     std::function<void (const boost::system::error_code&, std::size_t)> on_complete)
{
    boost::asio::async_write(*skt, boost::asio::buffer(*buffer), on_complete);
}

void tcpconnection::write_data(void* data_offset, std::size_t len,
                                     std::function<void (const boost::system::error_code&, std::size_t)> on_complete)
{
    boost::asio::async_write(*skt, boost::asio::buffer(data_offset, len), on_complete);
}

void tcpconnection::read_data(std::shared_ptr<std::vector<unsigned char>> buffer,
                                    std::size_t len,
                                    std::function<void(const boost::system::error_code&, std::size_t)> on_complete)
{
    boost::asio::async_read(*skt, boost::asio::buffer(*buffer), boost::asio::transfer_exactly(len), on_complete);
}

void tcpconnection::read_data(std::shared_ptr<std::vector<unsigned char>> buffer,
                                    std::function<void(const boost::system::error_code&, std::size_t)> on_complete)
{
    boost::asio::async_read(*skt, boost::asio::buffer(*buffer), on_complete);
}

std::unique_ptr<Iinfrastructure_timeout_connection> tcpconnection::timeout_timer(std::chrono::duration<int> timeout, std::function<void()> on_timeout)
{
    return std::unique_ptr<Iinfrastructure_timeout_connection> { new timeout_connection {context_, timeout, on_timeout}};
}

void tcpconnection::close()
{
    context_.post([this]() {
        skt->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
        skt->close();
      handler_end_connection(this);
   });
}







